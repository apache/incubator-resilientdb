pragma solidity >= 0.5.0;

// Transfer tokens from the contract owner
contract Token {
  mapping (address => uint256) balances;

  event Transfer(address indexed _from, address indexed _to, uint256 _value);
  
  constructor(uint256 s) public {
    balances[msg.sender] = s;
  }
  
  // Get the account balance of another account with address _owner
  function balanceOf(address _owner) public view returns (uint256) {
    return balances[_owner];
  }
  
  // Send _value amount of tokens to address _to
  function transfer(address _to, uint256 _value) public returns (bool) {
    if (balances[msg.sender] >= _value) {
      balances[msg.sender] -= _value;
      balances[_to] += _value;
      emit Transfer(msg.sender, _to, _value);
      return true;
    }
    else
    {
      return false;
    }
  }
}  
