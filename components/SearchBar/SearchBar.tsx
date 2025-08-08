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

'use client'

import { Button } from '@mantine/core';
import { IconSearch } from '@tabler/icons-react';

export function SearchBar() {
  const handleSearch = () => {
    // This triggers Nextra's built-in search using keyboard shortcut
    const event = new KeyboardEvent('keydown', {
      key: 'k',
      metaKey: true, // For Mac
      ctrlKey: true, // For Windows/Linux
    });
    document.dispatchEvent(event);
  };

  return (
    <Button
      onClick={handleSearch}
      leftSection={<IconSearch size={20} />}
      variant="subtle"
    >
      Search
    </Button>
  );
} 