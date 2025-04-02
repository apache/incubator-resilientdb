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
*
*/
const path = require('path');
require("dotenv").config({
    path: path.resolve(__dirname, '.env')
})
const cron = require('node-cron');
const axios = require('axios');
const sqlite3 = require('sqlite3').verbose();
const fs = require('fs');
const { getEnv } = require('../utils/envParser');
const logger = require('../utils/logger');


const BATCH_SIZE = 1000

// Create data directory if it doesn't exist
const dataDir = path.join(__dirname, '../cache');
if (!fs.existsSync(dataDir)) {
    fs.mkdirSync(dataDir, { recursive: true });
}

// Initialize SQLite database
const dbPath = path.join(dataDir, 'transactions.db');
const db = new sqlite3.Database(dbPath);



/**
 * Initialize the database schema
 */
function initializeDatabase() {
    return new Promise((resolve, reject) => {
        logger.info('Initializing transaction history database');
        
        try {
            db.serialize(() => {
                // Create transactions table
                db.run(`
                    CREATE TABLE IF NOT EXISTS transactions (
                        id INTEGER PRIMARY KEY AUTOINCREMENT,
                        block_id INTEGER NOT NULL,
                        volume INTEGER NOT NULL,
                        created_at TEXT NOT NULL
                    )
                `, (err) => {
                    if (err) {
                        logger.error('Error creating transactions table:', err);
                        reject(err);
                        return;
                    }
                    
                    // Create index on block_id
                    db.run('CREATE INDEX IF NOT EXISTS idx_transactions_block_id ON transactions(block_id)', (err) => {
                        if (err) {
                            logger.error('Error creating block_id index:', err);
                            reject(err);
                            return;
                        }
                        
                        // Create index on created_at for time-based queries
                        db.run('CREATE INDEX IF NOT EXISTS idx_transactions_created_at ON transactions(created_at)', (err) => {
                            if (err) {
                                logger.error('Error creating created_at index:', err);
                                reject(err);
                                return;
                            }
                            
                            // Verify database is accessible with a simple query
                            db.get('SELECT count(*) as count FROM transactions', (err, row) => {
                                if (err) {
                                    logger.error('Error verifying database access:', err);
                                    reject(err);
                                } else {
                                    logger.info(`Database initialized successfully. Current record count: ${row?.count || 0}`);
                                    resolve();
                                }
                            });
                        });
                    });
                });
            });
        } catch (err) {
            logger.error('Unexpected error during database initialization:', err);
            reject(err);
        }
    });
}

/**
 * Get the highest block ID we've processed so far
 */
function getLastProcessedBlockId() {
    return new Promise((resolve, reject) => {
        db.get('SELECT MAX(block_id) as last_id FROM transactions', (err, row) => {
            if (err) {
                logger.error('Error getting last processed block ID:', err);
                reject(err);
            } else {
                resolve(row?.last_id || 0);
            }
        });
    });
}

/**
 * Get the total number of blocks from the explorer
 */
async function getTotalBlocks() {
    try {
        const baseUrl = `${getEnv("EXPLORER_BASE_URL")}/populatetable`;
        const response = await axios.get(baseUrl);
        const data = (Array.isArray(response?.data) && response?.data.length > 0) ? response?.data[0] : { numBlocks: 0}
        return data?.blockNum || 0;
    } catch (error) {
        logger.error('Error fetching total blocks count:', error);
        throw error;
    }
}

/**
 * Fetch a range of blocks from the explorer
 */
async function fetchBlockRange(start, end) {
    try {
        const baseUrl = `${getEnv("EXPLORER_BASE_URL")}/v1/blocks/${start}/${end}`;
        const response = await axios.get(baseUrl);
        return response.data || [];
    } catch (error) {
        logger.error(`Error fetching blocks from ${start} to ${end}:`, error);
        throw error;
    }
}

/**
 * Store transactions from blocks in the database
 */
function storeTransactions(blocks) {
    return new Promise((resolve, reject) => {
        const timestamp = Math.floor(Date.now() / 1000);
        
        db.serialize(() => {
            const stmt = db.prepare(`
                INSERT INTO transactions 
                (block_id,volume,created_at) 
                VALUES (?, ?, ?)
            `);
            
            let transactionCount = 0;
            
            blocks.forEach(block => {
                const { id,createdAt,transactions } = block;
                    stmt.run(
                        id,
                        Array.isArray(transactions) ? transactions?.length : 0,
                        createdAt,
                    );
            });
            
            stmt.finalize(err => {
                if (err) {
                    logger.error('Error storing transactions:', err);
                    reject(err);
                } else {
                    logger.info(`Successfully stored ${transactionCount} transactions from ${blocks.length} blocks`);
                    resolve(transactionCount);
                }
            });
        });
    });
}

/**
 * Utility function to introduce a delay
 * 
 * @param {number} ms - Milliseconds to wait
 * @returns {Promise<void>} A promise that resolves after the specified time
 */
function sleep(ms) {
    return new Promise(resolve => setTimeout(resolve, ms));
}

/**
 * Main sync function to fetch and store transaction data
 */
async function syncTransactionHistory() {
    try {
        logger.info('Starting transaction history sync job');
        
        // Get current state
        const lastProcessedId = await getLastProcessedBlockId();
        const totalBlocks = await getTotalBlocks();
        
        logger.info(`Last processed block ID: ${lastProcessedId}, Total blocks: ${totalBlocks}`);
        
        if (lastProcessedId < totalBlocks) {
            const batchSize = BATCH_SIZE;
            let currentStart = lastProcessedId + 1;
            
            // Fetch and store blocks in batches
            while (currentStart <= totalBlocks) {
                const currentEnd = Math.min(currentStart + batchSize - 1, totalBlocks);
                logger.info(`Fetching blocks from ${currentStart} to ${currentEnd}`);
                
                const blocks = await fetchBlockRange(currentStart, currentEnd);
                await storeTransactions(blocks);

                // Add 1 second delay to avoid overwhelming the database or API
                await sleep(1000);
                
                currentStart = currentEnd + 1;
            }
            
            logger.info('Transaction history sync completed successfully');
        } else {
            logger.info('Transaction history is already up to date');
        }
    } catch (error) {
        logger.error('Error syncing transaction history:', error.message);
    }
}

/**
 * Initialize the service and start the cron job
 * 
 * @param {Object} options - Configuration options
 * @param {boolean} options.scheduleJob - Whether to schedule the recurring job
 * @param {string} options.cronSchedule - Cron schedule expression
 */
async function initSyncService(options = {}) {
    const { scheduleJob = true, cronSchedule = '0 * * * *' } = options;
    
    try {
        // Initialize database
        await initializeDatabase();
        
        // Run initial sync
        await syncTransactionHistory();
        
        // Schedule recurring job if requested
        if (scheduleJob) {
            cron.schedule(cronSchedule, async () => {
                logger.info(`Running scheduled transaction history sync (${cronSchedule})`);
                await syncTransactionHistory();
            });
            
            logger.info(`Transaction history sync service initialized and scheduled (${cronSchedule})`);
        } else {
            logger.info('Transaction history sync completed (one-time run)');
        }
    } catch (error) {
        logger.error('Failed to initialize transaction history sync service:', error);
    }
}

/**
 * Display the command usage information
 */
function showUsage() {
    console.log(`
Transaction History Sync Tool

Usage:
  node syncTransactionHistory.js [command] [options]

Commands:
  start       Start the service with scheduled cron job (default)
  run-once    Run the sync once without scheduling a cron job
  status      Show the current sync status
  help        Show this help message

Options:
  --schedule=<cron>    Custom cron schedule (default: "0 * * * *")
                       Example: --schedule="*/30 * * * *" for every 30 minutes

Examples:
  node syncTransactionHistory.js start
  node syncTransactionHistory.js run-once
  node syncTransactionHistory.js start --schedule="0 */2 * * *"
  node syncTransactionHistory.js status
    `);
}

/**
 * Check and display the current sync status
 */
async function showStatus() {
    try {
        const lastProcessedId = await getLastProcessedBlockId();
        const totalBlocks = await getTotalBlocks();
        const syncProgress = lastProcessedId > 0 
            ? ((lastProcessedId / totalBlocks) * 100).toFixed(2) 
            : 0;
            
        console.log('\nTransaction History Sync Status:');
        console.log('--------------------------------');
        console.log(`Last processed block ID:  ${lastProcessedId}`);
        console.log(`Total blocks available:   ${totalBlocks}`);
        console.log(`Sync progress:            ${syncProgress}%`);
        console.log(`Blocks remaining:         ${totalBlocks - lastProcessedId}`);
        console.log('--------------------------------\n');
    } catch (error) {
        logger.error('Error getting sync status:', error);
    }
}

// Process command line arguments
if (require.main === module) {
    const args = process.argv.slice(2);
    const command = args[0] || 'start';
    
    // Parse options
    const options = {};
    args.forEach(arg => {
        if (arg.startsWith('--schedule=')) {
            options.cronSchedule = arg.substring(11);
        }
    });
    
    // Execute appropriate command
    switch (command) {
        case 'start':
            logger.info('Starting transaction history sync service with scheduler');
            initSyncService(options);
            break;
        
        case 'run-once':
            logger.info('Running transaction history sync once');
            initSyncService({ ...options, scheduleJob: false });
            break;
            
        case 'status':
            showStatus();
            break;
            
        case 'help':
            showUsage();
            break;
            
        default:
            console.log(`Unknown command: ${command}`);
            showUsage();
            break;
    }
}
module.exports = {
    initSyncService,
    syncTransactionHistory,
    getLastProcessedBlockId,
    getTotalBlocks
};