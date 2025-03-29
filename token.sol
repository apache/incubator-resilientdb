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
