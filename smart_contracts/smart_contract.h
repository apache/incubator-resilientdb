#ifndef _SC_H_
#define _SC_H_
#include "global.h"
#include "wl.h"

#if BANKING_SMART_CONTRACT

class SmartContract
{
public:
    uint64_t execute();
    BSCType type;
};
/*
Transfer money form source account to the destination account
Transaction will abort in case that the source account doesn't have enough money
*/
class TransferMoneySmartContract : public SmartContract
{
public:
    uint64_t source_id;
    uint64_t dest_id;
    uint64_t amount;
    uint64_t execute();
};

/*
Deposit $amount$ money to the destination account
Transaction will always commit 
*/
class DepositMoneySmartContract : public SmartContract
{
public:
    uint64_t dest_id;
    uint64_t amount;
    uint64_t execute();
};

/*
Withdraw $amount$ money form source account 
Transaction will abort in case that the source account doesn't have enough money
*/
class WithdrawMoneySmartContract : public SmartContract
{
public:
    uint64_t source_id;
    uint64_t amount;
    uint64_t execute();
};

#endif
#endif
