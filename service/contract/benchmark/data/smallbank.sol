pragma solidity >= 0.5.0;

contract SmallBank {
    
    //uint constant MAX_ACCOUNT = 10000;
    //uint constant BALANCE = 10000;
    //bytes20 constant accountTab = "account";
    //bytes20 constant savingTab = "saving";
    //bytes20 constant checkingTab = "checking";
    
    mapping(address=>uint) savingStore;
    mapping(address=>uint) checkingStore;

    function almagate(address arg0, address arg1) public {
       uint bal1 = savingStore[arg0];
       uint bal2 = checkingStore[arg1];
       
       checkingStore[arg0] = 0;
       savingStore[arg1] = bal1 + bal2;
    }

    function getBalance(address arg0) public view returns (uint balance) {
        uint bal1 = savingStore[arg0];
        uint bal2 = checkingStore[arg0];
        
        balance = bal1 + bal2;
        return balance;
    }

    function resetBalance(address arg0, uint arg1, uint arg2) public returns (bool){
        checkingStore[arg0] = arg1;
        savingStore[arg0] = arg2;
        return true;
    }

    function updateBalance(address arg0, uint arg1) public returns (bool){
        uint bal1 = checkingStore[arg0];
        uint bal2 = arg1;
        
        checkingStore[arg0] = bal1 + bal2;
        return true;
    }
    
    function updateSaving(address arg0, uint arg1) public {
        uint bal1 = savingStore[arg0];
        uint bal2 = arg1;
        
        savingStore[arg0] = bal1 + bal2;
    }
    
    function sendPayment(address arg0, address arg1, uint arg2) public returns (bool){
        uint bal1 = checkingStore[arg0];
        uint bal2 = checkingStore[arg1];
        uint amount = arg2;
        if(bal1 <= amount){
          bal1 += 100000;
        }
        
        bal1 -= amount;
        bal2 += amount;
      
        checkingStore[arg0] = bal1;
        checkingStore[arg1] = bal2;
        return true;
    }
    
    function writeCheck(address arg0, uint arg1) public {
        uint bal1 = checkingStore[arg0];
        uint bal2 = savingStore[arg0];
        uint amount = arg1;
        
        if (amount < bal1 + bal2) {
            checkingStore[arg0] = bal1 - amount - 1;
        } 
        else {
            checkingStore[arg0] = bal1 - amount;
        }
    }
}
