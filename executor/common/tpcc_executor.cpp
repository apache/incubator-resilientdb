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


#include "executor/common/tpcc_executor.h"
#include <iostream>
#include <iomanip>
#include <glog/logging.h>

namespace resdb {

TpccExecutor::TpccExecutor() {
    db = loadDatabaseIntoMemory();
    if (!db) {
        LOG(ERROR) << "Fail to Load Database Into Memory";
        assert(false);
    }
}

TpccExecutor::~TpccExecutor() {
    sqlite3_close(db);
}

sqlite3* TpccExecutor::loadDatabaseIntoMemory() {
    sqlite3* diskDb = nullptr;
    sqlite3* memoryDb = nullptr;

    // Open the database file
    if (sqlite3_open(dbFilePath.c_str(), &diskDb) != SQLITE_OK) {
        LOG(ERROR) << "Failed to open disk database: " << sqlite3_errmsg(diskDb) << std::endl;
        return nullptr;
    }

    // Create an in-memory database
    if (sqlite3_open(":memory:", &memoryDb) != SQLITE_OK) {
        LOG(ERROR) << "Failed to create in-memory database: " << sqlite3_errmsg(memoryDb) << std::endl;
        sqlite3_close(diskDb);
        return nullptr;
    }

    // Use SQLite Backup API to copy data from disk to memory
    sqlite3_backup* backup = sqlite3_backup_init(memoryDb, "main", diskDb, "main");
    if (backup) {
        sqlite3_backup_step(backup, -1); // Copy all pages
        sqlite3_backup_finish(backup);
    } else {
        LOG(ERROR) << "Failed to initialize backup: " << sqlite3_errmsg(memoryDb) << std::endl;
    }

    sqlite3_close(diskDb);

    return memoryDb;
}


std::unique_ptr<std::string> TpccExecutor::ExecuteTransaction(TpccTransaction* tpcc_txn) {
    switch (tpcc_txn->type()) {
        case TpccTransaction::TPCC_TYPE_NEWORDER: 
            return std::make_unique<std::string>(ExecuteNewOrderTransaction(tpcc_txn->mutable_new_order()));

        case TpccTransaction::TPCC_TYPE_PAYMENT: 
            return std::make_unique<std::string>(ExecutePaymentTransaction(tpcc_txn->mutable_payment()));

        case TpccTransaction::TPCC_TYPE_ORDERSTATUS: 
            return std::make_unique<std::string>(ExecuteOrderStatusTransaction(tpcc_txn->mutable_order_status()));

        case TpccTransaction::TPCC_TYPE_DELIVERY: 
            return std::make_unique<std::string>(ExecuteDeliveryTransaction(tpcc_txn->mutable_delivery()));

        case TpccTransaction::TPCC_TYPE_STOCKLEVEL: 
            return std::make_unique<std::string>(ExecuteStockLevelTransaction(tpcc_txn->mutable_stock_level()));

        default:
            LOG(ERROR) << "WRONG TPCC TRANSACTION TYPE" << std::endl;
            assert(false);
            return nullptr;
    }
}

std::string TpccExecutor::ExecuteNewOrderTransaction(NewOrderTransaction* tpcc_txn) {
    int W_ID = tpcc_txn->w_id();
    int D_ID = tpcc_txn->d_id();
    int C_ID = tpcc_txn->c_id();
    int O_OL_CNT = tpcc_txn->o_ol_cnt();
    std::string DATETIME = tpcc_txn->date_time();
    std::vector<int> SUPWARE, ITEMID, QTY;
    SUPWARE.resize(O_OL_CNT);
    ITEMID.resize(O_OL_CNT);
    QTY.resize(O_OL_CNT);

    for (int i = 0; i < tpcc_txn->items_size(); ++i) {
        auto& item = tpcc_txn->items(i);
        // Access fields of OrderItem (example fields: ITEM_ID, SUPWARE, QTY)
        ITEMID[i] = item.i_id();     // Assuming OrderItem has an ITEM_ID field
        SUPWARE[i] = item.sup_w_id();     // Assuming OrderItem has a SUPWARE field
        QTY[i] = item.qty();             // Assuming OrderItem has a QTY field
    }

    return SQLITE3_NEW_ORDER(db, W_ID, D_ID, C_ID, O_OL_CNT, SUPWARE, ITEMID, QTY, DATETIME);
}

std::string TpccExecutor::ExecutePaymentTransaction(PaymentTransaction* tpcc_txn) {
    int W_ID = tpcc_txn->w_id(); 
    int D_ID = tpcc_txn->d_id(); 
    int C_W_ID = tpcc_txn->c_w_id(); 
    int C_D_ID = tpcc_txn->c_d_id(); 
    int C_ID = tpcc_txn->c_id(); 
    double H_AMOUNT = tpcc_txn->h_amount(); 
    std::string H_DATE = tpcc_txn->h_date(); 
    std::string C_LAST = tpcc_txn->c_last();  
    bool BYNAME = tpcc_txn->byname();  
    return SQLITE3_PAYMENT(db, W_ID, D_ID, C_W_ID, C_D_ID, C_ID, H_AMOUNT, H_DATE, C_LAST, BYNAME);
}

std::string TpccExecutor::ExecuteOrderStatusTransaction(OrderStatusTransaction* tpcc_txn) {
    int W_ID = tpcc_txn->w_id();
    int D_ID = tpcc_txn->d_id();
    int C_ID = tpcc_txn->c_id(); 
    std::string C_LAST = tpcc_txn->c_last();
    bool BYNAME = tpcc_txn->byname();
    return SQLITE3_ORDER_STATUS(db, W_ID, D_ID, C_ID, C_LAST, BYNAME);
}

std::string TpccExecutor::ExecuteDeliveryTransaction(DeliveryTransaction* tpcc_txn) {
    int W_ID = tpcc_txn->w_id();
    int CARRIER_ID = tpcc_txn->carrier_id();    // Carrier ID
    std::string DATE_TIME = tpcc_txn->date_time(); // Date and Tim
    return SQLITE3_DELIVERY(db, W_ID, CARRIER_ID, DATE_TIME);
}

std::string TpccExecutor::ExecuteStockLevelTransaction(StockLevelTransaction* tpcc_txn) {
    int W_ID = tpcc_txn->w_id();
    int D_ID = tpcc_txn->d_id();
    int THRESHOLD = tpcc_txn->threshold();
    return SQLITE3_STOCK_LEVEL(db, W_ID, D_ID, THRESHOLD);
}

std::string TpccExecutor::SQLITE3_NEW_ORDER(
    sqlite3* db, int W_ID, int D_ID, int C_ID, int O_OL_CNT,
    std::vector<int>& SUPWARE, std::vector<int>& ITEMID, std::vector<int>& QTY, std::string DATETIME
) {
    try {
        int O_ALL_LOCAL = 1;
        std::vector<double> PRICE;
        std::vector<std::string> INAME;
        std::vector<int> STOCK;
        std::vector<char> BG;
        std::vector<double> AMT; 
        double TOTAL;
        PRICE.resize(O_OL_CNT);
        INAME.resize(O_OL_CNT);
        STOCK.resize(O_OL_CNT);
        BG.resize(O_OL_CNT);
        AMT.resize(O_OL_CNT);

        sqlite3_stmt* stmt = nullptr;
        std::string query;
        double C_DISCOUNT, W_TAX, D_TAX;
        int D_NEXT_O_ID, O_ID;
        std::string C_LAST, C_CREDIT;
        double OL_AMOUNT, I_PRICE;
        std::string I_NAME, I_DATA, S_DATA;
        int OL_SUPPLY_W_ID, OL_I_ID, OL_QUANTITY, S_QUANTITY;

        // Start the transaction
        if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            return "Error: Failed to begin transaction.";
        }

        // Fetch customer and warehouse details
        query = "SELECT C_DISCOUNT, C_LAST, C_CREDIT, W_TAX FROM CUSTOMER, WAREHOUSE "
                "WHERE W_ID = ? AND C_W_ID = W_ID AND C_D_ID = ? AND C_ID = ?;";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw "SQL Error: Failed to prepare customer and warehouse details query.";
        }
        sqlite3_bind_int(stmt, 1, W_ID);
        sqlite3_bind_int(stmt, 2, D_ID);
        sqlite3_bind_int(stmt, 3, C_ID);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            C_DISCOUNT = sqlite3_column_double(stmt, 0);
            C_LAST = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            C_CREDIT = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            W_TAX = sqlite3_column_double(stmt, 3);
        } else {
            LOG(ERROR) << "W_ID: " << W_ID << " D_ID: " << D_ID << " C_ID: " << C_ID;
            throw "SQL Error: Unable to fetch customer and warehouse details.";
        }
        sqlite3_finalize(stmt);

        // Fetch district details
        query = "SELECT D_NEXT_O_ID, D_TAX FROM DISTRICT WHERE D_ID = ? AND D_W_ID = ?;";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw "SQL Error: Failed to prepare district details query.";
        }
        sqlite3_bind_int(stmt, 1, D_ID);
        sqlite3_bind_int(stmt, 2, W_ID);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            D_NEXT_O_ID = sqlite3_column_int(stmt, 0);
            D_TAX = sqlite3_column_double(stmt, 1);
        } else {
            throw "SQL Error: Unable to fetch district details.";
        }
        sqlite3_finalize(stmt);

        // Update district next order ID
        O_ID = D_NEXT_O_ID;
        query = "UPDATE DISTRICT SET D_NEXT_O_ID = ? WHERE D_ID = ? AND D_W_ID = ?;";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw "SQL Error: Failed to prepare update district next order ID query.";
        }
        sqlite3_bind_int(stmt, 1, D_NEXT_O_ID + 1);
        sqlite3_bind_int(stmt, 2, D_ID);
        sqlite3_bind_int(stmt, 3, W_ID);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw "SQL Error: Failed to update district next order ID.";
        }
        sqlite3_finalize(stmt);

        // Process each order line
        TOTAL = 0;

        for (int OL_NUMBER = 1; OL_NUMBER <= O_OL_CNT; OL_NUMBER++) {
            OL_SUPPLY_W_ID = SUPWARE[OL_NUMBER - 1];
            if (OL_SUPPLY_W_ID != W_ID) O_ALL_LOCAL = 0;

            OL_I_ID = ITEMID[OL_NUMBER - 1];
            OL_QUANTITY = QTY[OL_NUMBER - 1];

            // Fetch item details
            query = "SELECT I_PRICE, I_NAME, I_DATA FROM ITEM WHERE I_ID = ?;";
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                throw "SQL Error: Failed to prepare item details query.";
            }
            sqlite3_bind_int(stmt, 1, OL_I_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                I_PRICE = sqlite3_column_double(stmt, 0);
                I_NAME = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                I_DATA = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            } else {
                throw "SQL Error: Invalid item ID.";
            }
            sqlite3_finalize(stmt);

            PRICE[OL_NUMBER - 1] = I_PRICE;
            INAME[OL_NUMBER - 1] = I_NAME;

            // Fetch stock details
            query = "SELECT S_QUANTITY, S_DATA FROM STOCK WHERE S_I_ID = ? AND S_W_ID = ?;";
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                throw "SQL Error: Failed to prepare stock details query.";
            }
            sqlite3_bind_int(stmt, 1, OL_I_ID);
            sqlite3_bind_int(stmt, 2, OL_SUPPLY_W_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                S_QUANTITY = sqlite3_column_int(stmt, 0);
                S_DATA = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            } else {
                throw "SQL Error: Unable to fetch stock details.";
            }
            sqlite3_finalize(stmt);

            STOCK[OL_NUMBER - 1] = S_QUANTITY;
            BG[OL_NUMBER - 1] = (I_DATA.find("original") != std::string::npos && S_DATA.find("original") != std::string::npos) ? 'B' : 'G';

            if (S_QUANTITY > OL_QUANTITY) {
                S_QUANTITY -= OL_QUANTITY;
            } else {
                S_QUANTITY = S_QUANTITY - OL_QUANTITY + 91;
            }

            // Update stock quantity
            query = "UPDATE STOCK SET S_QUANTITY = ? WHERE S_I_ID = ? AND S_W_ID = ?;";
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                throw "SQL Error: Failed to prepare update stock quantity query.";
            }
            sqlite3_bind_int(stmt, 1, S_QUANTITY);
            sqlite3_bind_int(stmt, 2, OL_I_ID);
            sqlite3_bind_int(stmt, 3, OL_SUPPLY_W_ID);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw "SQL Error: Failed to update stock quantity.";
            }
            sqlite3_finalize(stmt);

            OL_AMOUNT = OL_QUANTITY * I_PRICE * (1 + W_TAX + D_TAX) * (1 - C_DISCOUNT);
            AMT[OL_NUMBER - 1] = OL_AMOUNT;
            TOTAL += OL_AMOUNT;

            // Insert into ORDER_LINE
            query = "INSERT INTO ORDER_LINE (OL_O_ID, OL_D_ID, OL_W_ID, OL_NUMBER, OL_I_ID, OL_SUPPLY_W_ID, OL_QUANTITY, OL_AMOUNT, OL_DELIVERY_D) VALUES (?, ?, ?, ?, ?, ?, ?, ?, 'null');";
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                throw "SQL Error: Failed to prepare insert into ORDER_LINE query.";
            }
            sqlite3_bind_int(stmt, 1, O_ID);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);
            sqlite3_bind_int(stmt, 4, OL_NUMBER);
            sqlite3_bind_int(stmt, 5, OL_I_ID);
            sqlite3_bind_int(stmt, 6, OL_SUPPLY_W_ID);
            sqlite3_bind_int(stmt, 7, OL_QUANTITY);
            sqlite3_bind_double(stmt, 8, OL_AMOUNT);

            if (sqlite3_step(stmt) != SQLITE_DONE) {
                throw "SQL Error: Failed to insert into ORDER_LINE.";
            }
            sqlite3_finalize(stmt);
        }

        // Insert into ORDERS
        query = "INSERT INTO ORDERS (O_ID, O_D_ID, O_W_ID, O_C_ID, O_ENTRY_D, O_OL_CNT, O_ALL_LOCAL) VALUES (?, ?, ?, ?, ?, ?, ?);";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw "SQL Error: Failed to prepare insert into ORDERS query.";
        }
        sqlite3_bind_int(stmt, 1, O_ID);
        sqlite3_bind_int(stmt, 2, D_ID);
        sqlite3_bind_int(stmt, 3, W_ID);
        sqlite3_bind_int(stmt, 4, C_ID);
        sqlite3_bind_text(stmt, 5, DATETIME.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 6, O_OL_CNT);
        sqlite3_bind_int(stmt, 7, O_ALL_LOCAL);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw "SQL Error: Failed to insert into ORDERS.";
        }
        sqlite3_finalize(stmt);

        // Insert into NEW_ORDER
        query = "INSERT INTO NEW_ORDER (NO_O_ID, NO_D_ID, NO_W_ID) VALUES (?, ?, ?);";
        if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw "SQL Error: Failed to prepare insert into NEW_ORDER query.";
        }
        sqlite3_bind_int(stmt, 1, O_ID);
        sqlite3_bind_int(stmt, 2, D_ID);
        sqlite3_bind_int(stmt, 3, W_ID);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw "SQL Error: Failed to insert into NEW_ORDER.";
        }
        sqlite3_finalize(stmt);

        // Commit transaction
        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw "SQL Error: Failed to commit transaction.";
        }

        // Prepare the result string
        std::ostringstream result;
        result << "Order Summary:\n";
        result << "O_ID: " << O_ID << "\n";
        result << "DATETIME: " << DATETIME << "\n";
        result << "O_ALL_LOCAL: " << O_ALL_LOCAL << "\n";
        result << "Line Items:\n";
        for (int i = 0; i < O_OL_CNT; ++i) {
            result << "  Item " << (i + 1) << ":\n";
            result << "    SUPWARE: " << SUPWARE[i] << "\n";
            result << "    ITEMID: " << ITEMID[i] << "\n";
            result << "    QTY: " << QTY[i] << "\n";
            result << "    PRICE: " << std::fixed << std::setprecision(2) << PRICE[i] << "\n";
            result << "    INAME: " << INAME[i] << "\n";
            result << "    BG: " << BG[i] << "\n";
            result << "    OL_AMOUNT: " << std::fixed << std::setprecision(2) << AMT[i] << "\n";
        }
        result << "TOTAL: " << std::fixed << std::setprecision(2) << TOTAL << "\n";

        return result.str();
    } catch (const char* msg) {
        std::cerr << msg << std::endl;
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::string("Error: ") + msg;
    }
}


std::string TpccExecutor::SQLITE3_PAYMENT(sqlite3* db, int W_ID, int D_ID, int C_W_ID, int C_D_ID, int C_ID, double H_AMOUNT, std::string H_DATE, std::string C_LAST, bool BYNAME) {
    try {
        // Start the transaction
        if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            std::string error_msg = "Failed to begin transaction: " + std::string(sqlite3_errmsg(db));
            return error_msg;
        }

        sqlite3_stmt* stmt;
        int NAMECNT = 0;

        // Update WAREHOUSE
        std::string query = "UPDATE WAREHOUSE SET W_YTD = W_YTD + ? WHERE W_ID = ?;";
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_double(stmt, 1, H_AMOUNT);
        sqlite3_bind_int(stmt, 2, W_ID);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string error_msg = "SQL Error: Unable to update warehouse. " + std::string(sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return error_msg;
        }
        sqlite3_finalize(stmt);

        // Fetch WAREHOUSE DATA
        std::string W_STREET_1, W_STREET_2, W_CITY, W_STATE, W_ZIP, W_NAME;
        query = "SELECT W_STREET_1, W_STREET_2, W_CITY, W_STATE, W_ZIP, W_NAME FROM WAREHOUSE WHERE W_ID = ?;";
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, W_ID);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            W_STREET_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            W_STREET_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            W_CITY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            W_STATE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            W_ZIP = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            W_NAME = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        } else {
            sqlite3_finalize(stmt);
            return "SQL Error: Failed to fetch WAREHOUSE data.";
        }
        sqlite3_finalize(stmt);

        // Update DISTRICT
        query = "UPDATE DISTRICT SET D_YTD = D_YTD + ? WHERE D_W_ID = ? AND D_ID = ?;";
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_double(stmt, 1, H_AMOUNT);
        sqlite3_bind_int(stmt, 2, W_ID);
        sqlite3_bind_int(stmt, 3, D_ID);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string error_msg = "SQL Error: Unable to update district. " + std::string(sqlite3_errmsg(db));
            sqlite3_finalize(stmt);
            return error_msg;
        }
        sqlite3_finalize(stmt);

        // Fetch DISTRICT DATA
        std::string D_STREET_1, D_STREET_2, D_CITY, D_STATE, D_ZIP, D_NAME;
        query = "SELECT D_STREET_1, D_STREET_2, D_CITY, D_STATE, D_ZIP, D_NAME FROM DISTRICT WHERE D_W_ID = ? AND D_ID=?;";
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, W_ID);
        sqlite3_bind_int(stmt, 2, D_ID);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            D_STREET_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            D_STREET_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            D_CITY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            D_STATE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
            D_ZIP = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
            D_NAME = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        } else {
            fflush(stdout);
            throw "SQL Error: Payment(): Fail to Fetch WAREHOUSE DATA.";
        }
        sqlite3_finalize(stmt);

        std::string C_FIRST, C_MIDDLE, C_STREET_1, C_STREET_2, C_CITY, C_STATE, C_ZIP, C_PHONE, C_CREDIT, C_SINCE;
        double C_CREDIT_LIM, C_DISCOUNT, C_BALANCE;

        if (BYNAME) {
            // Step 1: Get the number of customers with the same last name
            query = "SELECT COUNT(C_ID) FROM CUSTOMER WHERE C_LAST = ? AND C_D_ID = ? AND C_W_ID = ?;";
            sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, C_LAST.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                NAMECNT = sqlite3_column_int(stmt, 0);
            } else {
                throw std::runtime_error("SQL Error: Unable to fetch customer count.");
            }
            sqlite3_finalize(stmt);

            // Step 2: Fetch midpoint customer details
            query = "SELECT C_ID, C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, C_STATE, C_ZIP, C_PHONE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, C_BALANCE, C_SINCE FROM CUSTOMER WHERE C_LAST = ? AND C_D_ID = ? AND C_W_ID = ? ORDER BY C_FIRST;";
            sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, C_LAST.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);

            if (NAMECNT % 2 == 1) NAMECNT++;
            for (int N = 0; N < NAMECNT / 2; ++N) {
                if (sqlite3_step(stmt) == SQLITE_ROW) {
                C_ID = sqlite3_column_int(stmt, 0);
                C_FIRST = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                C_MIDDLE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                C_LAST = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                C_STREET_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                C_STREET_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                C_CITY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
                C_STATE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
                C_ZIP = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
                C_PHONE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
                C_CREDIT = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
                C_CREDIT_LIM = sqlite3_column_double(stmt, 11);
                C_DISCOUNT = sqlite3_column_double(stmt, 12);
                C_BALANCE = sqlite3_column_double(stmt, 13);
                C_SINCE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
                } else {
                    throw std::runtime_error("SQL Error: Unable to fetch customer details.");
                }
            }
            sqlite3_finalize(stmt);
        } 
        else {
            query = "SELECT C_FIRST, C_MIDDLE, C_LAST, C_STREET_1, C_STREET_2, C_CITY, C_STATE, C_ZIP, C_PHONE, C_CREDIT, C_CREDIT_LIM, C_DISCOUNT, C_BALANCE, C_SINCE FROM CUSTOMER WHERE C_W_ID = ? AND C_D_ID = ? AND C_ID = ?";
            sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, C_W_ID);
            sqlite3_bind_int(stmt, 2, C_D_ID);
            sqlite3_bind_int(stmt, 3, C_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                C_FIRST = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
                C_MIDDLE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                C_LAST = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                C_STREET_1 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
                C_STREET_2 = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
                C_CITY = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
                C_STATE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
                C_ZIP = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
                C_PHONE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
                C_CREDIT = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
                C_CREDIT_LIM = sqlite3_column_double(stmt, 10);
                C_DISCOUNT = sqlite3_column_double(stmt, 11);
                C_BALANCE = sqlite3_column_double(stmt, 12);
                C_SINCE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 13));
            } else {
                std::cerr << "No midpoint customer found." << std::endl;
            }

            sqlite3_finalize(stmt);
        }

        if (C_CREDIT == "BC") {
            std::string C_DATA;
            query = "SELECT C_DATA FROM CUSTOMER WHERE C_W_ID = ? AND C_D_ID = ? AND C_ID = ?;";
            sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, C_W_ID);
            sqlite3_bind_int(stmt, 2, C_D_ID);
            sqlite3_bind_int(stmt, 3, C_ID);
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                C_DATA = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
            } else {
                std::cerr << "Fail to find c_data with C_ID " << C_ID  << std::endl;
            }
            sqlite3_finalize(stmt);
            std::string C_NEW_DATA = createNewData(C_ID, C_D_ID, C_W_ID, D_ID, W_ID, H_AMOUNT, C_DATA);

            // Update CUSTOMER balance
            query = "UPDATE CUSTOMER SET C_BALANCE = C_BALANCE + ? , C_DATA = ? WHERE C_W_ID = ? AND C_D_ID = ? AND C_ID = ?;";
            sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_double(stmt, 1, H_AMOUNT);
            sqlite3_bind_text(stmt, 2, C_NEW_DATA.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, W_ID);
            sqlite3_bind_int(stmt, 4, D_ID);
            sqlite3_bind_int(stmt, 5, C_ID);
        } else {
            // Update CUSTOMER balance
            query = "UPDATE CUSTOMER SET C_BALANCE = C_BALANCE + ? WHERE C_W_ID = ? AND C_D_ID = ? AND C_ID = ?;";
            sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_double(stmt, 1, H_AMOUNT);
            sqlite3_bind_int(stmt, 2, C_W_ID);
            sqlite3_bind_int(stmt, 3, C_D_ID);
            sqlite3_bind_int(stmt, 4, C_ID);
        }

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error("SQL Error: Unable to update customer balance.");
        }
        sqlite3_finalize(stmt);

        // Insert into HISTORY
        query = "INSERT INTO HISTORY (H_C_D_ID, H_C_W_ID, H_C_ID, H_D_ID, H_W_ID, H_DATE, H_AMOUNT, H_DATA) VALUES (?, ?, ?, ?, ?, ?, ?, 'Payment');";
        std::string H_DATA = W_NAME + "\0" + D_NAME + "    ";
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, C_D_ID);
        sqlite3_bind_int(stmt, 2, C_W_ID);
        sqlite3_bind_int(stmt, 3, C_ID);
        sqlite3_bind_int(stmt, 4, D_ID);
        sqlite3_bind_int(stmt, 5, W_ID);
        sqlite3_bind_text(stmt, 6, H_DATE.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_double(stmt, 7, H_AMOUNT);
        sqlite3_bind_text(stmt, 8, H_DATA.c_str(), -1, SQLITE_STATIC);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error("SQL Error: Unable to insert into history.");
        }
        sqlite3_finalize(stmt);

        // Commit transaction
        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            std::string error_msg = "SQL Error: Failed to commit transaction. " + std::string(sqlite3_errmsg(db));
            return error_msg;
        }
        // Success message
        return "Payment processed successfully for Customer " + std::to_string(C_ID) + ": " + C_LAST + ".\n";  
    } catch (const std::exception& e) {
        // Rollback transaction on error
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return "Error: " + std::string(e.what());  // Return the error message
    }
}



std::string TpccExecutor::SQLITE3_ORDER_STATUS(sqlite3* db, int W_ID, int D_ID, int C_ID, const std::string& C_LAST, bool BYNAME) {
    std::ostringstream result;
    try {
        // Start the transaction
        if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to begin transaction: " + std::string(sqlite3_errmsg(db)));
        }

        sqlite3_stmt* stmt = nullptr;
        int NAMECNT = 0;
        double C_BALANCE = 0.0;
        std::string C_FIRST, C_MIDDLE;
        int O_ID = 0, O_CARRIER_ID = 0;
        std::string ENTDATE;
        std::vector<int> OL_I_ID, OL_SUPPLY_W_ID, OL_QUANTITY;
        std::vector<double> OL_AMOUNT;
        std::vector<std::string> OL_DELIVERY_D;

        if (BYNAME) {
            // Step 1: Get the count of customers with the same last name
            const std::string query_count = "SELECT COUNT(C_ID) FROM CUSTOMER WHERE C_LAST = ? AND C_D_ID = ? AND C_W_ID = ?;";
            if (sqlite3_prepare_v2(db, query_count.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                throw std::runtime_error("SQL Error: Failed to prepare query_count.");
            }
            sqlite3_bind_text(stmt, 1, C_LAST.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                NAMECNT = sqlite3_column_int(stmt, 0);
            } else {
                throw std::runtime_error("SQL Error: Unable to fetch customer count.");
            }
            sqlite3_finalize(stmt);

            if (NAMECNT == 0) return "No customers found with the given last name.";

            // Step 2: Fetch the midpoint customer
            int midpoint = (NAMECNT - 1) / 2; // Midpoint (0-indexed)
            const std::string query_customer = "SELECT C_BALANCE, C_FIRST, C_MIDDLE, C_ID FROM CUSTOMER WHERE C_LAST = ? AND C_D_ID = ? AND C_W_ID = ? ORDER BY C_FIRST LIMIT 1 OFFSET ?;";
            sqlite3_prepare_v2(db, query_customer.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, C_LAST.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);
            sqlite3_bind_int(stmt, 4, midpoint);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                C_BALANCE = sqlite3_column_double(stmt, 0);
                C_FIRST = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                C_MIDDLE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
                C_ID = sqlite3_column_int(stmt, 3);
            } else {
                throw std::runtime_error("SQL Error: Unable to fetch midpoint customer.");
            }
            sqlite3_finalize(stmt);
        } else {
            // Fetch customer details by C_ID
            const std::string query_customer_by_id = "SELECT C_BALANCE, C_FIRST, C_MIDDLE FROM CUSTOMER WHERE C_ID = ? AND C_D_ID = ? AND C_W_ID = ?;";
            sqlite3_prepare_v2(db, query_customer_by_id.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, C_ID);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                C_BALANCE = sqlite3_column_double(stmt, 0);
                C_FIRST = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
                C_MIDDLE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
            } else {
                throw std::runtime_error("SQL Error: Unable to fetch customer details.");
            }
            sqlite3_finalize(stmt);
        }

        result << "Customer: " << C_FIRST << " " << C_MIDDLE << " " << C_LAST
               << "\nBalance: " << C_BALANCE << "\n";

        // Step 3: Get the most recent order
        const std::string query_order = "SELECT O_ID, O_CARRIER_ID, O_ENTRY_D FROM ORDERS WHERE O_D_ID = ? AND O_W_ID = ? AND O_C_ID = ? ORDER BY O_ID DESC LIMIT 1;";
        sqlite3_prepare_v2(db, query_order.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, D_ID);
        sqlite3_bind_int(stmt, 2, W_ID);
        sqlite3_bind_int(stmt, 3, C_ID);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            O_ID = sqlite3_column_int(stmt, 0);
            O_CARRIER_ID = sqlite3_column_int(stmt, 1);
            ENTDATE = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        } else {
            throw std::runtime_error("SQL Error: Unable to fetch most recent order.");
        }
        sqlite3_finalize(stmt);

        result << "Most Recent Order: " << O_ID
               << "\nCarrier ID: " << O_CARRIER_ID
               << "\nEntry Date: " << ENTDATE << "\n";

        // Step 4: Get order line details
        const std::string query_order_line = "SELECT OL_I_ID, OL_SUPPLY_W_ID, OL_QUANTITY, OL_AMOUNT, OL_DELIVERY_D FROM ORDER_LINE WHERE OL_O_ID = ? AND OL_D_ID = ? AND OL_W_ID = ?;";
        sqlite3_prepare_v2(db, query_order_line.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, O_ID);
        sqlite3_bind_int(stmt, 2, D_ID);
        sqlite3_bind_int(stmt, 3, W_ID);

        result << "Order Lines:\n";
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            result << "  Item ID: " << sqlite3_column_int(stmt, 0)
                   << ", Supply W_ID: " << sqlite3_column_int(stmt, 1)
                   << ", Quantity: " << sqlite3_column_int(stmt, 2)
                   << ", Amount: " << sqlite3_column_double(stmt, 3)
                   << ", Delivery Date: " << reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4)) << "\n";
        }
        sqlite3_finalize(stmt);

        // Commit transaction
        sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr);
    } catch (const std::exception& E) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::string("Error: ") + E.what();
    }

    return result.str();
}



std::string TpccExecutor::SQLITE3_DELIVERY(sqlite3* db, int W_ID, int O_CARRIER_ID, const std::string& DATETIME) {
    try {
        // Start the transaction
        if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            return std::string("Failed to begin transaction: ") + sqlite3_errmsg(db);
        }

        sqlite3_stmt* stmt = nullptr;
        int D_ID, NO_O_ID, C_ID;
        double OL_TOTAL;

        for (D_ID = 1; D_ID <= 10; ++D_ID) {
            // Step 1: Find the oldest unprocessed order
            const std::string query1 = "SELECT NO_O_ID FROM NEW_ORDER WHERE NO_D_ID = ? AND NO_W_ID = ? ORDER BY NO_O_ID ASC LIMIT 1;";
            if (sqlite3_prepare_v2(db, query1.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
                return std::string("SQL Error: Unable to prepare query1.");
            }
            sqlite3_bind_int(stmt, 1, D_ID);
            sqlite3_bind_int(stmt, 2, W_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                NO_O_ID = sqlite3_column_int(stmt, 0);
            } else {
                sqlite3_finalize(stmt);
                continue;  // No new orders in this district
            }
            sqlite3_finalize(stmt);

            // Step 2: Delete the new order
            const std::string query2 = "DELETE FROM NEW_ORDER WHERE NO_O_ID = ? AND NO_D_ID = ? AND NO_W_ID = ?;";
            sqlite3_prepare_v2(db, query2.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, NO_O_ID);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                return std::string("SQL Error: Failed to delete from NEW_ORDER.");
            }
            sqlite3_finalize(stmt);

            // Step 3: Get the customer ID from the order
            const std::string query3 = "SELECT O_C_ID FROM ORDERS WHERE O_ID = ? AND O_D_ID = ? AND O_W_ID = ?;";
            sqlite3_prepare_v2(db, query3.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, NO_O_ID);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                C_ID = sqlite3_column_int(stmt, 0);
            } else {
                sqlite3_finalize(stmt);
                return std::string("SQL Error: Unable to fetch customer ID.");
            }
            sqlite3_finalize(stmt);

            // Step 4: Update the order with the carrier ID
            const std::string query4 = "UPDATE ORDERS SET O_CARRIER_ID = ? WHERE O_ID = ? AND O_D_ID = ? AND O_W_ID = ?;";
            sqlite3_prepare_v2(db, query4.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, O_CARRIER_ID);
            sqlite3_bind_int(stmt, 2, NO_O_ID);
            sqlite3_bind_int(stmt, 3, D_ID);
            sqlite3_bind_int(stmt, 4, W_ID);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                return std::string("SQL Error: Failed to update carrier ID.");
            }
            sqlite3_finalize(stmt);

            // Step 5: Update order line with delivery date
            const std::string query5 = "UPDATE ORDER_LINE SET OL_DELIVERY_D = ? WHERE OL_O_ID = ? AND OL_D_ID = ? AND OL_W_ID = ?;";
            sqlite3_prepare_v2(db, query5.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_text(stmt, 1, DATETIME.c_str(), -1, SQLITE_TRANSIENT);
            sqlite3_bind_int(stmt, 2, NO_O_ID);
            sqlite3_bind_int(stmt, 3, D_ID);
            sqlite3_bind_int(stmt, 4, W_ID);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                return std::string("SQL Error: Failed to update delivery date.");
            }
            sqlite3_finalize(stmt);

            // Step 6: Calculate total order line amount
            const std::string query6 = "SELECT SUM(OL_AMOUNT) FROM ORDER_LINE WHERE OL_O_ID = ? AND OL_D_ID = ? AND OL_W_ID = ?;";
            sqlite3_prepare_v2(db, query6.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_int(stmt, 1, NO_O_ID);
            sqlite3_bind_int(stmt, 2, D_ID);
            sqlite3_bind_int(stmt, 3, W_ID);

            if (sqlite3_step(stmt) == SQLITE_ROW) {
                OL_TOTAL = sqlite3_column_double(stmt, 0);
            } else {
                sqlite3_finalize(stmt);
                return std::string("SQL Error: Unable to calculate total.");
            }
            sqlite3_finalize(stmt);

            // Step 7: Update the customer's balance
            const std::string query7 = "UPDATE CUSTOMER SET C_BALANCE = C_BALANCE + ? WHERE C_ID = ? AND C_D_ID = ? AND C_W_ID = ?;";
            sqlite3_prepare_v2(db, query7.c_str(), -1, &stmt, nullptr);
            sqlite3_bind_double(stmt, 1, OL_TOTAL);
            sqlite3_bind_int(stmt, 2, C_ID);
            sqlite3_bind_int(stmt, 3, D_ID);
            sqlite3_bind_int(stmt, 4, W_ID);
            if (sqlite3_step(stmt) != SQLITE_DONE) {
                sqlite3_finalize(stmt);
                return std::string("SQL Error: Failed to update customer balance.");
            }
            sqlite3_finalize(stmt);
        }

        // Commit transaction
        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            return std::string("SQL Error: Failed to commit transaction.");
        }

        return "DELIVERY by CARRIER " + std::to_string(O_CARRIER_ID) + " is completed successfully.";

    } catch (const std::exception& E) {
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        return std::string("Error: ") + E.what();
    }
}



std::string TpccExecutor::SQLITE3_STOCK_LEVEL(sqlite3* db, int W_ID, int D_ID, int THRESHOLD) {
    try {
        // Start the transaction
        if (sqlite3_exec(db, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw std::runtime_error(std::string("Failed to begin transaction: ") + sqlite3_errmsg(db));
        }

        sqlite3_stmt* stmt;
        int O_ID;
        int STOCK_COUNT;

        // Step 1: Retrieve the next order ID from the district
        std::string query = "SELECT D_NEXT_O_ID FROM DISTRICT WHERE D_W_ID = ? AND D_ID = ?;";
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, W_ID);
        sqlite3_bind_int(stmt, 2, D_ID);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            O_ID = sqlite3_column_int(stmt, 0);
        } else {
            sqlite3_finalize(stmt);
            throw std::runtime_error("SQL Error: Unable to fetch next order ID.");
        }
        sqlite3_finalize(stmt);

        // Step 2: Count distinct stock items below the threshold
        query = "SELECT COUNT(DISTINCT S_I_ID) "
                "FROM ORDER_LINE, STOCK "
                "WHERE OL_W_ID = ? AND OL_D_ID = ? "
                "AND OL_O_ID < ? AND OL_O_ID >= ? "
                "AND S_W_ID = ? AND S_I_ID = OL_I_ID "
                "AND S_QUANTITY < ?;";
        sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, W_ID);
        sqlite3_bind_int(stmt, 2, D_ID);
        sqlite3_bind_int(stmt, 3, O_ID);
        sqlite3_bind_int(stmt, 4, O_ID - 20); // Use calculated range
        sqlite3_bind_int(stmt, 5, W_ID);
        sqlite3_bind_int(stmt, 6, THRESHOLD);

        if (sqlite3_step(stmt) == SQLITE_ROW) {
            STOCK_COUNT = sqlite3_column_int(stmt, 0);
        } else {
            sqlite3_finalize(stmt);
            throw std::runtime_error("SQL Error: Unable to count stock items below threshold.");
        }
        sqlite3_finalize(stmt);

        // Commit transaction
        if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
            throw std::runtime_error("SQL Error: Failed to commit transaction.");
        }

        // Return the stock count as a string
        return "Stock Level: " + std::to_string(STOCK_COUNT);

    } catch (const std::exception& E) {
        // Rollback transaction on error
        sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        // Return the error message as a string
        return std::string("Error: ") + E.what();
    }
}


std::string TpccExecutor::createNewData(int c_id, int c_d_id, int c_w_id, int d_id, int w_id, 
                   double h_amount, const std::string& c_data) {
    // Using std::ostringstream for formatted strings
    std::ostringstream oss;
    oss << "| " 
        << std::setw(4) << c_id << " "
        << std::setw(2) << c_d_id << " "
        << std::setw(4) << c_w_id << " "
        << std::setw(2) << d_id << " "
        << std::setw(4) << w_id << " $"
        << std::fixed << std::setprecision(2) << std::setw(7) << h_amount << " ";

    std::string c_new_data = oss.str();

    // Ensure the length of c_new_data does not exceed 500 after concatenation
    if (c_new_data.length() < 500) {
        c_new_data += c_data.substr(0, 500 - c_new_data.length());
    }

    return c_new_data;
}



}