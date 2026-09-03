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

