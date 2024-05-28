pragma solidity >= 0.5.0;

// Transfer tokens from the contract owner
contract KV {
  mapping (address => uint256) balances;
  mapping (address => uint256) allow;

  constructor(uint256 s) public {
    balances[msg.sender] = s;
  }
  
  // Get the account balance of another account with address _owner
  function get(address _owner) public view returns (uint256) {
    return balances[_owner];
  }
  
  // Send _value amount of tokens to address _to
  function set(address _to, uint256 _value) public returns (bool) {
      uint256 values = balances[_to];
      balances[_to] = _value + values;
      return true;
  }

  // Send _value amount of tokens to address _to
  function transfer(address _from, address _to, uint256 _value) public returns (bool) {
      if (balances[_from] > _value) {
        balances[_from] -= _value;
      }
      balances[_to] += _value;
      return true;
  }

  function transferif(address _from, address _to1, address _to2, uint256 _value) public returns (bool) {
      uint256 value = balances[_from];

      if (balances[_from] > _value) {
        balances[_from] -= _value;
      }
      if (value < 800 ) {
        balances[_to1] += _value;
      } else {
        balances[_to2] += _value;
      }
      return true;
  }

  function transferto(address _to1, address _to2, uint256 _value) public returns (bool) {
      uint256 value = balances[msg.sender];
      balances[msg.sender] -= _value;
      if (value < 800 ) {
        balances[_to1] += _value;
      } else {
        balances[_to2] += _value;
      }
      return true;
  }

}  

