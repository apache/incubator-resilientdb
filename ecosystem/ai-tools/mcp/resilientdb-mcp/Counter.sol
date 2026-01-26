// SPDX-License-Identifier: MIT
pragma solidity ^0.8.0;

contract Counter {
    int256 private count;
    address public owner;
    
    event CountChanged(int256 newCount, address changedBy);
    
    constructor(int256 initialCount) {
        count = initialCount;
        owner = msg.sender;
    }
    
    function increment() public {
        count += 1;
        emit CountChanged(count, msg.sender);
    }
    
    function decrement() public {
        count -= 1;
        emit CountChanged(count, msg.sender);
    }
    
    function getCount() public view returns (int256) {
        return count;
    }
    
    function reset() public {
        require(msg.sender == owner, "Only owner can reset");
        count = 0;
        emit CountChanged(count, msg.sender);
    }
    
    function setCount(int256 newCount) public {
        require(msg.sender == owner, "Only owner can set count");
        count = newCount;
        emit CountChanged(count, msg.sender);
    }
}