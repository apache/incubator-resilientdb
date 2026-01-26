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

export const defaultcomparisonPeriod = {
  label: '24 hours prior',
  ms: 86400 * 1000,
};

export const comparisonPeriods = [
  [
    {
      label: '1 hour prior',
      ms: 3600 * 1000,
    },
    {
      label: '12 hours prior',
      ms: 43200 * 1000,
    },
    defaultcomparisonPeriod,
  ],
  [
    {
      label: '1 week prior',
      ms: 604800 * 1000,
    },
    {
      label: '2 weeks prior',
      ms: 1209600 * 1000,
    },
    {
      label: '30 days prior',
      ms: 2592000 * 1000,
    },
  ],
];
