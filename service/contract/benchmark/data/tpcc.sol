pragma solidity >= 0.5.0;

contract TPCC{
    
    mapping(address=>uint) warehouse_ytd;
    mapping(address=>uint) darehouse_ytd;
    mapping(address=>uint) carehouse_ytd;
    mapping(address=>mapping(address=>uint)) district_ytd;
    mapping(address=>mapping(address=>mapping(address=>uint))) balance;
    mapping(address=>mapping(address=>mapping(address=>uint))) custom_ytd;
    mapping(address=>mapping(address=>mapping(address=>uint))) payment_cnt;

    function payment(address w_id, address d_id, address c_id, uint amount) public returns (bool){
      //warehouse_ytd[w_id] += amount;
      //darehouse_ytd[d_id] += amount;
      //carehouse_ytd[c_id] += amount;
      warehouse_ytd[w_id] += amount;
      district_ytd[w_id][d_id] += amount;
    
      if(balance[w_id][d_id][c_id]<amount){
        balance[w_id][d_id][c_id] += 100000;
      }
      else {
        balance[w_id][d_id][c_id] -= amount;
      }
      custom_ytd[w_id][d_id][c_id] += amount;
      payment_cnt[w_id][d_id][c_id] += 1;
      return true;
    }
}
