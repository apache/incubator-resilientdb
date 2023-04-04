pragma solidity >= 0.5.0;

// Transfer tokens from the contract owner
contract KV {
  mapping (address => uint256) balances;

  constructor() public {
  }
  
  // Get the account balance of another account with address _owner
  function get(address _owner) public view returns (uint256) {
    return balances[_owner];
  }
  
  // Send _value amount of tokens to address _to
  function set(address _to, uint256 _value) public returns (bool) {
      balances[_to] = _value;
      return true;
  }
}  
