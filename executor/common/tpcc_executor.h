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

#pragma once

#include "sqlite3.h"
#include "platform/proto/resdb.pb.h"

namespace resdb {
    
class TpccExecutor {
    public:
        TpccExecutor();
        ~TpccExecutor();
        std::unique_ptr<std::string> ExecuteTransaction(TpccTransaction* tpcc_txn);

    private:
        sqlite3* loadDatabaseIntoMemory();

        std::string ExecuteNewOrderTransaction(NewOrderTransaction* tpcc_txn);
        std::string ExecutePaymentTransaction(PaymentTransaction* tpcc_txn);
        std::string ExecuteOrderStatusTransaction(OrderStatusTransaction* tpcc_txn);
        std::string ExecuteDeliveryTransaction(DeliveryTransaction* tpcc_txn);
        std::string ExecuteStockLevelTransaction(StockLevelTransaction* tpcc_txn);

        std::string SQLITE3_NEW_ORDER(sqlite3* db, int W_ID, int D_ID, int C_ID, int O_OL_CNT, std::vector<int>& SUPWARE, std::vector<int>& ITEMID, std::vector<int>& QTY, std::string DATETIME);
        std::string SQLITE3_PAYMENT(sqlite3* db, int W_ID, int D_ID, int C_W_ID, int C_D_ID, int C_ID, double H_AMOUNT, std::string H_DATE, std::string C_LAST, bool BYNAME);
        std::string SQLITE3_ORDER_STATUS(sqlite3* db, int W_ID, int D_ID, int C_ID, const std::string& C_LAST, bool BYNAME);
        std::string SQLITE3_DELIVERY(sqlite3* db, int W_ID, int O_CARRIER_ID, const std::string& DATETIME);
        std::string SQLITE3_STOCK_LEVEL(sqlite3* db, int W_ID, int D_ID, int THRESHOLD);

        std::string createNewData(int c_id, int c_d_id, int c_w_id, int d_id, int w_id, 
                   double h_amount, const std::string& c_data);

        sqlite3* db = nullptr;
        const std::string& dbFilePath = "./tpcc.db";
};

}

