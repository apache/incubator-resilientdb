#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#

import asyncio
import json
import logging
from typing import Optional

import httpx
import motor.motor_asyncio
import websockets
from pyee import AsyncIOEventEmitter
from pymongo import UpdateOne

from .config import MongoConfig, ResilientDBConfig
from .exceptions import ResilientPythonCacheError

logging.basicConfig(level=logging.INFO, format='%(message)s')
logger = logging.getLogger(__name__)

logging.getLogger("httpx").setLevel(logging.WARNING)
logging.getLogger("httpcore").setLevel(logging.WARNING)

class ResilientPythonCache(AsyncIOEventEmitter):
    def __init__(self, mongo_config: MongoConfig, resilient_db_config: ResilientDBConfig):
        super().__init__()
        self.mongo_config = mongo_config
        self.resilient_db_config = resilient_db_config
        self.http_endpoint: str
        self.ws_endpoint: str
        self.reconnect_interval: int
        self.fetch_interval: int
        self.current_block_number: int = 0
        self.fetch_task: Optional[asyncio.Task] = None
        self.ws_task: Optional[asyncio.Task] = None
        self.is_closing: bool = False
        self.reconnect_attempts: int = 0

        self.initialize_endpoints()

    def initialize_endpoints(self):
        config = self.resilient_db_config
        self.reconnect_interval = config.reconnect_interval
        self.fetch_interval = config.fetch_interval

        if not config.base_url.startswith("resilientdb://"):
            raise ResilientPythonCacheError("Invalid protocol in base_url. Expected 'resilientdb://'")

        url = config.base_url[len("resilientdb://"):]
        hostname_port = url.split("/")[0]
        hostname, port = (hostname_port.split(":") + [None])[:2]
        port = port or ""

        http_protocol = "https" if config.http_secure else "http"
        ws_protocol = "wss" if config.ws_secure else "ws"

        if port:
            host_port = f"{hostname}:{port}"
        else:
            host_port = hostname

        self.http_endpoint = config.http_endpoint or f"{http_protocol}://{host_port}/v1/blocks"
        self.ws_endpoint = config.ws_endpoint or f"{ws_protocol}://{host_port}/blockupdatelistener"

        logger.info(f"HTTP Endpoint: {self.http_endpoint}")
        logger.info(f"WebSocket Endpoint: {self.ws_endpoint}")

    async def initialize(self):
        try:
            self.mongo_client = motor.motor_asyncio.AsyncIOMotorClient(self.mongo_config.uri)
            self.db = self.mongo_client[self.mongo_config.db_name]
            self.collection = self.db[self.mongo_config.collection_name]

            await self.mongo_client.admin.command('ping')
            logger.info(f"Connected to MongoDB database: {self.mongo_config.db_name}, "
                        f"collection: {self.mongo_config.collection_name}")

            await self.fetch_and_sync_initial_blocks()

            self.fetch_task = asyncio.create_task(self.start_periodic_fetch())
            self.ws_task = asyncio.create_task(self.connect_websocket())

        except Exception as e:
            logger.error("Error initializing connections:")
            logger.error(e)
            raise ResilientPythonCacheError(str(e)) from e

    async def fetch_and_sync_initial_blocks(self):
        try:
            last_block = await self.collection.find_one({}, sort=[("id", -1)])
            self.current_block_number = last_block['id'] if last_block and 'id' in last_block else 0

            batch_size = 100
            concurrency_limit = 5

            batch_ranges = []
            blocks_fetched = True

            async with httpx.AsyncClient() as client:
                while blocks_fetched:
                    min_seq = self.current_block_number + 1
                    max_seq = min_seq + batch_size - 1

                    url = f"{self.http_endpoint}/{min_seq}/{max_seq}"
                    logger.info(f"Checking for blocks from {min_seq} to {max_seq}")
                    response = await client.get(url)
                    blocks = response.json()

                    if isinstance(blocks, list) and blocks:
                        self.current_block_number = max(block.get('id', 0) for block in blocks)
                        batch_ranges.append({'min_seq': min_seq, 'max_seq': max_seq})
                    else:
                        blocks_fetched = False

            if not batch_ranges:
                logger.info("No new blocks to sync.")
                return

            logger.info(f"Total batches to fetch: {len(batch_ranges)}")

            semaphore = asyncio.Semaphore(concurrency_limit)
            tasks = [self.fetch_and_sync_batch(batch['min_seq'], batch['max_seq'], semaphore)
                     for batch in batch_ranges]
            await asyncio.gather(*tasks)

        except Exception as e:
            logger.error("Error fetching initial blocks:")
            logger.error(e)
            raise ResilientPythonCacheError(str(e)) from e

    async def fetch_and_sync_batch(self, min_seq: int, max_seq: int, semaphore: asyncio.Semaphore):
        async with semaphore:
            try:
                async with httpx.AsyncClient() as client:
                    url = f"{self.http_endpoint}/{min_seq}/{max_seq}"
                    logger.info(f"Fetching blocks from {min_seq} to {max_seq}")
                    response = await client.get(url)
                    blocks = response.json()

                if isinstance(blocks, list) and blocks:
                    processed_blocks = self.process_blocks(blocks)
                    bulk_ops = [
                        UpdateOne(
                            {'id': block['id']},
                            {'$set': block},
                            upsert=True
                        ) for block in processed_blocks
                    ]

                    if bulk_ops:
                        result = await self.collection.bulk_write(bulk_ops)
                        logger.info(f"Blocks {min_seq} to {max_seq} synced: "
                                    f"Inserted {result.upserted_count}, Modified {result.modified_count}")
                        self.emit('data', processed_blocks)  # Emit 'data' event with new blocks
                else:
                    logger.info(f"No blocks fetched for range {min_seq} to {max_seq}")
            except Exception as e:
                logger.error(f"Error fetching blocks from {min_seq} to {max_seq}:")
                logger.error(e)

    def process_blocks(self, blocks: list) -> list:
        for block in blocks:
            transactions = block.get('transactions', [])
            if isinstance(transactions, list):
                for transaction in transactions:
                    if 'value' in transaction and isinstance(transaction['value'], str):
                        value = transaction['value'].strip()
                        if (value.startswith('{') and value.endswith('}')) or \
                           (value.startswith('[') and value.endswith(']')):
                            try:
                                transaction['value'] = json.loads(value)
                            except json.JSONDecodeError:
                                logger.warning(f"Could not parse transaction value for key {transaction.get('key')}. "
                                               f"Keeping original value.")
        return blocks

    async def start_periodic_fetch(self):
        logger.info(f"Started periodic fetch every {self.fetch_interval / 1000} seconds.")
        try:
            while not self.is_closing:
                await self.fetch_and_sync_new_blocks()
                await asyncio.sleep(self.fetch_interval / 1000)
        except asyncio.CancelledError:
            pass  # Allow task to be cancelled gracefully

    async def fetch_and_sync_new_blocks(self):
        try:
            batch_size = 100
            concurrency_limit = 5
            batch_ranges = []
            blocks_fetched = True

            async with httpx.AsyncClient() as client:
                while blocks_fetched:
                    min_seq = self.current_block_number + 1
                    max_seq = min_seq + batch_size - 1

                    url = f"{self.http_endpoint}/{min_seq}/{max_seq}"
                    logger.info(f"Checking for new blocks from {min_seq} to {max_seq}")
                    response = await client.get(url)
                    blocks = response.json()

                    if isinstance(blocks, list) and blocks:
                        self.current_block_number = max(block.get('id', 0) for block in blocks)
                        batch_ranges.append({'min_seq': min_seq, 'max_seq': max_seq})
                    else:
                        blocks_fetched = False

            if not batch_ranges:
                logger.info("No new blocks to sync.")
                return

            logger.info(f"Total new batches to fetch: {len(batch_ranges)}")

            semaphore = asyncio.Semaphore(concurrency_limit)
            tasks = [self.fetch_and_sync_batch(batch['min_seq'], batch['max_seq'], semaphore)
                     for batch in batch_ranges]
            await asyncio.gather(*tasks)

        except Exception as e:
            logger.error("Error fetching new blocks:")
            logger.error(e)
            self.emit('error', e)

    async def connect_websocket(self):
        try:
            async with websockets.connect(self.ws_endpoint) as websocket:
                logger.info(f"Connected to WebSocket: {self.ws_endpoint}")
                self.reconnect_attempts = 0
                self.emit('connected')
                # Removed the duplicate log statement here
                # logger.info("WebSocket connected.")

                async for message in websocket:
                    logger.info(f"Received message: {message}")
                    try:
                        parsed_message = json.loads(message)
                    except json.JSONDecodeError:
                        parsed_message = {"type": message}

                    if parsed_message.get("type") == "Update blocks":
                        await self.fetch_and_sync_new_blocks()
                    else:
                        logger.warning(f"Received unrecognized message: {message}")
        except (websockets.exceptions.ConnectionClosedError,
                websockets.exceptions.InvalidURI,
                websockets.exceptions.InvalidHandshake) as e:
            logger.warning(f"WebSocket closed or failed to connect: {e}")
            if not self.is_closing:
                await self.attempt_reconnection()
        except asyncio.CancelledError:
            pass  # Allow task to be cancelled gracefully
        except Exception as e:
            logger.error("WebSocket error:")
            logger.error(e)
            if not self.is_closing:
                await self.attempt_reconnection()

    async def attempt_reconnection(self):
        delay = self.reconnect_interval * (2 ** self.reconnect_attempts) / 1000  # Convert ms to seconds
        logger.info(f"Attempting to reconnect in {delay} seconds...")
        await asyncio.sleep(delay)
        self.reconnect_attempts += 1
        logger.info(f"Reconnection attempt #{self.reconnect_attempts}")
        self.ws_task = asyncio.create_task(self.connect_websocket())

    async def close(self):
        try:
            self.is_closing = True
            if self.fetch_task:
                self.fetch_task.cancel()
                try:
                    await self.fetch_task
                except asyncio.CancelledError:
                    pass

            if self.ws_task:
                self.ws_task.cancel()
                try:
                    await self.ws_task
                except asyncio.CancelledError:
                    pass

            await self.mongo_client.close()
            logger.info("Closed MongoDB and WebSocket connections.")
            self.emit('closed')
        except Exception as e:
            logger.error("Error closing connections:")
            logger.error(e)
            raise ResilientPythonCacheError(str(e)) from e