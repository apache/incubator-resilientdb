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

const path = require('path');

module.exports = {
  extends: [path.join(__dirname, '../.eslintrc.js')],
  ignorePatterns: ['public', 'javascript/util', '*.spec.*', '.eslintrc.js'],

  rules: {
    // https://github.com/import-js/eslint-plugin-import/issues/1650
    'import/no-extraneous-dependencies': [
      'error',
      {
        packageDir: [process.cwd(), path.resolve(__dirname, '../')],
      },
    ],
    // since we use immutablejs in the reducer
    'no-param-reassign': 'off',
  },
  parserOptions: {
    tsconfigRootDir: __dirname,
  },
};
