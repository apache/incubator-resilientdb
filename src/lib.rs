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

/// This modeule provides acces to other methods in the rest of the modules.
pub mod resdb;

/// This module provides access to the functions interfacing with the transaction endpoint.
pub mod transaction;

/// This module provides access to the functions interfacing with the block endpoint.
pub mod blocks;

/// This module provides access to functions that allow user to generate public-private keys and hash data.
pub mod crypto;

/// Re-export ResDB from the resdb module for convenient use.
pub use resdb::ResDB;
