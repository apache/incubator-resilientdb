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

import express from 'express';
import { graphqlHTTP } from 'express-graphql';
import schema from './schema.js';
import logger from './logger.js';

const app = express();

app.use('/graphql', graphqlHTTP({
  schema,
  graphiql: true,
}));

app.get('/', (req, res) => {
  res.send('Welcome to the Smart Contracts GraphQL API');
});

app.use((err, req, res, next) => {
  logger.error(`Express error: ${err.message}`);
  res.status(500).send('Internal Server Error');
});

const PORT = process.env.PORT || 8400;

app.listen(PORT, () => {
  console.log(`Server is running on port ${PORT}...`);
});
