#include "platform/consensus/ordering/common/framework/tpcc.h"
#include <glog/logging.h>
namespace resdb {
namespace common {

TpccTransactionGenerator::TpccTransactionGenerator() {
    // Initialize random seed
    std::srand(std::time(nullptr)); // Seed with current time
    gen = std::mt19937(rd());
}
    
int TpccTransactionGenerator::fillTpccTransaction(Request* request) {
    int x = generateRandomInt(1, 100);
    if (x <= 45) {
        // std::cout << "NEWORDER\n";
        request->mutable_tpcc_txn()->set_type(TpccTransaction::TPCC_TYPE_NEWORDER);
        auto* new_order = request->mutable_tpcc_txn()->mutable_new_order();
        int value = fillNewOrderTransaction(new_order);
        // LOG(ERROR) << "W_ID: " << request->tpcc_txn().new_order().w_id();
        return 0 + value;
    }
    else if (x <= 45 + 43) {
        // std::cout << "PAYMENT\n";
        request->mutable_tpcc_txn()->set_type(TpccTransaction::TPCC_TYPE_PAYMENT);
        auto* payment = request->mutable_tpcc_txn()->mutable_payment();
        // LOG(ERROR) << "TYPE4: " << request->type();
        return -10 + fillPaymentTransaction(payment);
    }
    else if (x <= 45 + 43 + 4) {
        // std::cout << "ORDER_STATUS\n";
        request->mutable_tpcc_txn()->set_type(TpccTransaction::TPCC_TYPE_ORDERSTATUS);
        auto* order_status = request->mutable_tpcc_txn()->mutable_order_status();
        return -20 + fillOrderStatusTransaction(order_status);
    }
    else if (x <= 45 + 43 + 4 + 4) {
        // std::cout << "DELIVERY\n";
        request->mutable_tpcc_txn()->set_type(TpccTransaction::TPCC_TYPE_DELIVERY);
        auto* delivery = request->mutable_tpcc_txn()->mutable_delivery();
        return -30 + fillDeliveryTransaction(delivery);
    }
    else if (x <= 100) {
        // std::cout << "STOCK_LEVEL\n";
        request->mutable_tpcc_txn()->set_type(TpccTransaction::TPCC_TYPE_STOCKLEVEL);
        auto* stock_level = request->mutable_tpcc_txn()->mutable_stock_level();
        return -40 + fillStockLevelTransaction(stock_level);
    }
    return 0;
}

int TpccTransactionGenerator::fillNewOrderTransaction(resdb::NewOrderTransaction* txn) {
    int W_ID = 1;
    int D_ID = generateRandomInt(1, 10);
    int C_ID = generateRandomInt(1, 3000);
    int O_OL_CNT = generateRandomInt(5, 15);
    std::string DATETIME = getCurrentTimestamp();

    std::vector<int> SUPWARE; 
    std::vector<int> ITEMID;
    std::vector<int> QTY;
    SUPWARE.resize(O_OL_CNT);
    ITEMID.resize(O_OL_CNT);
    QTY.resize(O_OL_CNT);

    txn->set_w_id(W_ID); // Random warehouse ID
    txn->set_d_id(D_ID); // Random district ID
    txn->set_c_id(C_ID); // Random customer ID
    txn->set_o_ol_cnt(O_OL_CNT); // Random customer ID
    txn->set_date_time(DATETIME); // Random customer ID

    for(int i = 0; i < O_OL_CNT; i++) {
        SUPWARE[i] = generateRandomInt(1, 100) > 1 ? W_ID : 1;
        ITEMID[i] = NURand (8191,1,100000);
        QTY[i] = generateRandomInt(1, 10);
    }

    // Add some random items
    for (int i = 0; i < O_OL_CNT; ++i) {
        auto* item = txn->add_items();
        item->set_i_id(ITEMID[i]); // Random item ID
        item->set_sup_w_id(SUPWARE[i]); // Random supplier warehouse ID
        item->set_qty(QTY[i]); // Random quantity
    }

    return 0;
}

int TpccTransactionGenerator::fillPaymentTransaction(resdb::PaymentTransaction* txn) {
    int C_W_ID, C_D_ID, W_ID, D_ID, C_ID;
    std::string C_LAST, H_DATE;
    bool BYNAME;
    double H_AMOUNT;

    C_ID = 0;
    C_LAST = "";
    C_W_ID = W_ID = 1;

    int x = generateRandomInt(1, 100);
    int y = generateRandomInt(1, 100);
    D_ID = generateRandomInt(1, 10);

    if (x > 85) {
        C_D_ID = generateRandomInt(1, 10);
    } else {
        C_D_ID = D_ID;
    }

    if (y <= 60) {
        BYNAME = true;
        C_LAST = generateCustomerLastName(NURand(255,0,999));
    } else {
        BYNAME = false;
        C_ID = NURand(1023,1,3000);
    }
    H_DATE = getCurrentTimestamp();
    H_AMOUNT = generateRandomDouble(1, 5000);

    
    txn->set_w_id(W_ID);
    txn->set_d_id(D_ID);
    txn->set_c_w_id(C_W_ID);
    txn->set_c_d_id(C_D_ID);
    txn->set_c_id(C_ID);
    txn->set_h_amount(H_AMOUNT); 
    txn->set_h_date(H_DATE);
    txn->set_c_last(C_LAST); 
    txn->set_byname(BYNAME); 

    return 0;
}

int TpccTransactionGenerator::fillOrderStatusTransaction(resdb::OrderStatusTransaction* txn) {
    int W_ID, D_ID, C_ID;
    std::string C_LAST;
    bool BYNAME;
    C_ID = 0;
    C_LAST = "LAST";
    int y = generateRandomInt(1, 100);
    W_ID = 1;
    D_ID = generateRandomInt(1, 10);

    if (y <= 60) {
        BYNAME = true;
        C_LAST = generateCustomerLastName(NURand(255,0,999));
    } else {
        BYNAME = false;
        C_ID = NURand(1023,1,3000);
    }

    txn->set_w_id(W_ID);
    txn->set_d_id(D_ID);
    txn->set_c_id(C_ID);
    txn->set_c_last(C_LAST); 
    txn->set_byname(BYNAME); 

    return 0;
}

int TpccTransactionGenerator::fillDeliveryTransaction(resdb::DeliveryTransaction* txn) {
    int W_ID = 1;
    int O_CARRIER_ID = generateRandomInt(1, 10);
    std::string DATETIME = getCurrentTimestamp(); 

    txn->set_w_id(W_ID);
    txn->set_carrier_id(O_CARRIER_ID);
    txn->set_date_time(DATETIME);

    return 0;
}


int TpccTransactionGenerator::fillStockLevelTransaction(resdb::StockLevelTransaction* txn) {
    int W_ID = 1;
    int D_ID = generateRandomInt(1, 10);
    int THRESHOLD = generateRandomInt(10, 20);

    txn->set_w_id(W_ID);
    txn->set_d_id(D_ID);
    txn->set_threshold(THRESHOLD); // Random stock level threshold

    return 0;
}
   
} // common
}  // namespace resdb
