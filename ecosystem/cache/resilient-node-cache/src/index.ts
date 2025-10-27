/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

import WebSocket from 'ws';
import axios from 'axios';
import {
  MongoClient,
  Collection,
  Db,
} from 'mongodb';
import EventEmitter from 'events';

export interface MongoConfig {
  uri: string;
  dbName: string;
  collectionName: string;
}

export interface ResilientDBConfig {
  baseUrl: string; // e.g., 'resilientdb://localhost:18000'
  httpSecure?: boolean; // optional, default is false
  wsSecure?: boolean; // optional, default is false
  httpEndpoint?: string; // optional, overrides the default constructed endpoint
  wsEndpoint?: string; // optional, overrides the default constructed endpoint
  reconnectInterval?: number; // in milliseconds, optional
  fetchInterval?: number; // in milliseconds, optional
}

export interface IncomingData {
  [key: string]: any;
}

export class WebSocketMongoSync extends EventEmitter {
    public mongoClient: MongoClient;
    public db!: Db;
    public collection!: Collection;
    private ws!: WebSocket;
    private mongoConfig: MongoConfig;
    private resilientDBConfig: ResilientDBConfig;
    private httpEndpoint!: string;
    private wsEndpoint!: string;
    private reconnectInterval!: number;
    private reconnectAttempts: number = 0;
    private isClosing: boolean = false;
    private currentBlockNumber: number = 0;
    private fetchInterval!: number;
    private fetchIntervalId?: NodeJS.Timeout;

    constructor(mongoConfig: MongoConfig, resilientDBConfig: ResilientDBConfig) {
        super();
        this.mongoConfig = mongoConfig;
        this.resilientDBConfig = resilientDBConfig;
        this.mongoClient = new MongoClient(this.mongoConfig.uri);

        this.initializeEndpoints();
    }

    private initializeEndpoints(): void {
        const {
        baseUrl,
        httpSecure = false,
        wsSecure = false,
        httpEndpoint,
        wsEndpoint,
        reconnectInterval = 5000,
        fetchInterval = 30000,
        } = this.resilientDBConfig;
        this.reconnectInterval = reconnectInterval;
        this.fetchInterval = fetchInterval;

        const url = new URL(baseUrl);

        if (url.protocol !== 'resilientdb:') {
        throw new Error('Invalid protocol in baseUrl. Expected resilientdb://');
        }

        const hostname = url.hostname;
        const port = url.port;
        const hostPort = port ? `${hostname}:${port}` : hostname;

        const httpProtocol = httpSecure ? 'https' : 'http';
        const wsProtocol = wsSecure ? 'wss' : 'ws';

        this.httpEndpoint = httpEndpoint || `${httpProtocol}://${hostPort}/v1/blocks`;
        this.wsEndpoint = wsEndpoint || `${wsProtocol}://${hostPort}/blockupdatelistener`;

        console.log(`HTTP Endpoint: ${this.httpEndpoint}`);
        console.log(`WebSocket Endpoint: ${this.wsEndpoint}`);
    }

    public async initialize(): Promise<void> {
        try {
        await this.mongoClient.connect();
        this.db = this.mongoClient.db(this.mongoConfig.dbName);
        this.collection = this.db.collection(this.mongoConfig.collectionName);
        console.log(
            `Connected to MongoDB database: ${this.mongoConfig.dbName}, collection: ${this.mongoConfig.collectionName}`
        );

        await this.fetchAndSyncInitialBlocks();

        this.startPeriodicFetch();

        this.connectWebSocket();
        } catch (error) {
        console.error('Error initializing connections:', error);
        throw error;
        }
    }

    private async fetchAndSyncInitialBlocks(): Promise<void> {
        try {
        const lastBlock = await this.collection.findOne({}, { sort: { id: -1 } });
        if (lastBlock && lastBlock.id) {
            this.currentBlockNumber = lastBlock.id;
        } else {
            this.currentBlockNumber = 0;
        }

        const batchSize = 100;
        const concurrencyLimit = 5;

        let blocksFetched = true;
        const batchRanges = [];

        while (blocksFetched) {
            const min_seq = this.currentBlockNumber + 1;
            const max_seq = min_seq + batchSize - 1;

            const url = `${this.httpEndpoint}/${min_seq}/${max_seq}`;
            console.log(`Checking for blocks from ${min_seq} to ${max_seq}`);
            const response = await axios.get(url);
            const blocks: IncomingData[] = response.data;

            if (Array.isArray(blocks) && blocks.length > 0) {
            this.currentBlockNumber = Math.max(
                this.currentBlockNumber,
                ...blocks.map((block) => block.id || 0)
            );
            batchRanges.push({ min_seq, max_seq });
            } else {
            blocksFetched = false;
            }
        }

        if (batchRanges.length === 0) {
            console.log('No new blocks to sync.');
            return;
        }

        console.log(`Total batches to fetch: ${batchRanges.length}`);

        // Implement a simple concurrency limiter
        await this.processBatchRanges(batchRanges, concurrencyLimit);
        } catch (error) {
        console.error('Error fetching initial blocks:', error);
        throw error;
        }
    }

    private async processBatchRanges(batchRanges: { min_seq: number; max_seq: number }[], concurrencyLimit: number): Promise<void> {
        const queue = batchRanges.slice(); // Clone the array
        const promises: Promise<void>[] = [];

        const worker = async (): Promise<void> => {
        while (queue.length > 0) {
            const { min_seq, max_seq } = queue.shift()!;
            await this.fetchAndSyncBatch(min_seq, max_seq);
        }
        };

        for (let i = 0; i < concurrencyLimit; i++) {
        promises.push(worker());
        }

        await Promise.all(promises);
    }

    private async fetchAndSyncBatch(min_seq: number, max_seq: number): Promise<void> {
        try {
        const url = `${this.httpEndpoint}/${min_seq}/${max_seq}`;
        console.log(`Fetching blocks from ${min_seq} to ${max_seq}`);
        const response = await axios.get(url);
        const blocks: IncomingData[] = response.data;

        if (Array.isArray(blocks) && blocks.length > 0) {
            const processedBlocks = this.processBlocks(blocks);

            const bulkOps = processedBlocks.map((block) => ({
            updateOne: {
                filter: { id: block.id },
                update: { $set: block },
                upsert: true,
            },
            }));

            if (bulkOps.length > 0) {
            const bulkWriteResult = await this.collection.bulkWrite(bulkOps);
            console.log(
                `Blocks ${min_seq} to ${max_seq} synced: Inserted ${bulkWriteResult.upsertedCount}, Modified ${bulkWriteResult.modifiedCount}`
            );
            }
        } else {
            console.log(`No blocks fetched for range ${min_seq} to ${max_seq}`);
        }
        } catch (error) {
        console.error(`Error fetching blocks from ${min_seq} to ${max_seq}:`, error);
        }
    }

    private startPeriodicFetch(): void {
        this.fetchIntervalId = setInterval(() => {
        this.fetchAndSyncNewBlocks();
        }, this.fetchInterval);
        console.log(`Started periodic fetch every ${this.fetchInterval / 1000} seconds.`);
    }

    private stopPeriodicFetch(): void {
        if (this.fetchIntervalId) {
        clearInterval(this.fetchIntervalId);
        console.log('Stopped periodic fetch.');
        }
    }

    private connectWebSocket(): void {
        this.ws = new WebSocket(this.wsEndpoint);

        this.ws.on('open', () => {
        console.log(`Connected to WebSocket: ${this.wsEndpoint}`);
        this.reconnectAttempts = 0;
        this.emit('connected');
        });

        this.ws.on('message', async (data: WebSocket.Data) => {
        try {
            const message = typeof data === 'string' ? data : data.toString();
            console.log('Received message:', message);

            let parsedMessage: { type: string; data?: any };
            try {
            parsedMessage = JSON.parse(message);
            } catch (e) {
            parsedMessage = { type: message };
            }

            if (parsedMessage.type === 'Update blocks') {
            await this.fetchAndSyncNewBlocks();
            } else {
            console.warn('Received unrecognized message:', message);
            }
        } catch (error) {
            console.error('Error processing message:', error);
            this.emit('error', error);
        }
        });

        this.ws.on('error', (error) => {
        console.error('WebSocket error:', error);
        this.emit('error', error);
        });

        this.ws.on('close', (code, reason) => {
        console.warn(`WebSocket closed. Code: ${code}, Reason: ${reason}`);
        this.emit('disconnected', { code, reason });
        if (!this.isClosing) {
            this.attemptReconnection();
        }
        });
    }

    private async fetchAndSyncNewBlocks(): Promise<void> {
        try {
        const batchSize = 100;
        const concurrencyLimit = 5;
        let blocksFetched = true;
        const batchRanges = [];

        while (blocksFetched) {
            const min_seq = this.currentBlockNumber + 1;
            const max_seq = min_seq + batchSize - 1;

            const url = `${this.httpEndpoint}/${min_seq}/${max_seq}`;
            console.log(`Checking for new blocks from ${min_seq} to ${max_seq}`);
            const response = await axios.get(url);
            const blocks: IncomingData[] = response.data;

            if (Array.isArray(blocks) && blocks.length > 0) {
            this.currentBlockNumber = Math.max(
                this.currentBlockNumber,
                ...blocks.map((block) => block.id || 0)
            );
            batchRanges.push({ min_seq, max_seq });
            } else {
            blocksFetched = false;
            }
        }

        if (batchRanges.length === 0) {
            console.log('No new blocks to sync.');
            return;
        }

        console.log(`Total new batches to fetch: ${batchRanges.length}`);

        await this.processBatchRanges(batchRanges, concurrencyLimit);
        } catch (error) {
        console.error('Error fetching new blocks:', error);
        this.emit('error', error);
        }
    }

    private processBlocks(blocks: IncomingData[]): IncomingData[] {
        return blocks.map((block) => {
        if (Array.isArray(block.transactions)) {
            block.transactions = block.transactions.map((transaction: any) => {
            if (transaction && transaction.hasOwnProperty('value')) {
                const value = transaction.value;
                if (typeof value === 'string') {
                const trimmedValue = value.trim();
                if (
                    (trimmedValue.startsWith('{') && trimmedValue.endsWith('}')) ||
                    (trimmedValue.startsWith('[') && trimmedValue.endsWith(']'))
                ) {
                    try {
                    transaction.value = JSON.parse(value);
                    } catch (error) {
                    console.warn(
                        `Could not parse transaction value for key ${transaction.key}. Keeping original value.`
                    );
                    }
                } else {
                    transaction.value = value;
                }
                } else {
                transaction.value = value;
                }
            }
            return transaction;
            });
        }
        return block;
        });
    }

    private attemptReconnection(): void {
        const reconnectInterval = this.reconnectInterval;

        const delay = reconnectInterval * Math.pow(2, this.reconnectAttempts);
        console.log(`Attempting to reconnect in ${delay / 1000} seconds...`);
        setTimeout(() => {
        this.reconnectAttempts += 1;
        console.log(`Reconnection attempt #${this.reconnectAttempts}`);
        this.connectWebSocket();
        }, delay);
    }

    public async close(): Promise<void> {
        try {
        this.isClosing = true;

        if (this.ws) {
            const readyState = this.ws.readyState;
            if (readyState === WebSocket.CONNECTING) {
            this.ws.on('open', () => {
                this.ws.close();
            });
            } else if (readyState === WebSocket.OPEN) {
            this.ws.close();
            }
        }

        this.stopPeriodicFetch();
        await this.mongoClient.close();
        console.log('Closed MongoDB and WebSocket connections.');
        this.emit('closed');
        } catch (error) {
        console.error('Error closing connections:', error);
        throw error;
        }
    }
}