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

/* eslint-disable import/prefer-default-export */
import { store } from '@webapp/ui/Notifications';
import type { NotificationOptions } from '@webapp/ui/Notifications';
import { createAsyncThunk } from '../async-thunk';

export const addNotification = createAsyncThunk(
  'notifications/add',
  async (opts: NotificationOptions) => {
    return new Promise((resolve) => {
      // TODO:
      // we can at some point add default buttons OK and Cancel
      // which would resolve/reject the promise
      store.addNotification({
        ...opts,
        onRemoval: () => {
          // TODO: fix type
          resolve(null as ShamefulAny);
        },
      });
    });
  }
);

// TODO
// create a store with maintains the notification history?
