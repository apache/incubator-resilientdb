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

import * as fs from 'fs';
import * as path from 'path';

const filePath = path.join(__dirname, 'index.js');
const shebang = '#!/usr/bin/env node\n';

fs.readFile(filePath, 'utf8', (err, data) => {
  if (err) {
    console.error('Error reading file:', err);
    process.exit(1);
  }

  if (data.startsWith(shebang)) {
    console.log('Shebang already exists.');
    process.exit(0);
  }

  const updatedData = shebang + data;

  fs.writeFile(filePath, updatedData, 'utf8', (err) => {
    if (err) {
      console.error('Error writing file:', err);
      process.exit(1);
    }
    console.log('Build generated successfully.');
  });
});