pragma solidity >= 0.5.0;

// Transfer tokens from the contract owner
contract ycsb {
  mapping (uint256 => uint256) balances;

  // Get the account balance of another account with address _owner
  function get(uint256 key) public view returns (uint256) {
    return balances[key];
  }
  
  // Send _value amount of tokens to address _to
  function set(uint256 key, uint256 value) public returns (bool) {
      balances[key] = value;
      return true;
  }
}  

