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

import { renderDiff, RenderDiffResponse } from '@webapp/services/render';
import { RequestAbortedError } from '@webapp/services/base';
import { addNotification } from '../notifications';
import { createAsyncThunk } from '../../async-thunk';
import { ContinuousState } from './state';

let diffViewAbortController: AbortController | undefined;

export const fetchDiffView = createAsyncThunk<
  { profile: RenderDiffResponse },
  {
    leftQuery: string;
    leftFrom: string;
    leftUntil: string;
    rightQuery: string;
    rightFrom: string;
    rightUntil: string;
  },
  { state: { continuous: ContinuousState } }
>('continuous/diffView', async (params, thunkAPI) => {
  if (diffViewAbortController) {
    diffViewAbortController.abort();
  }

  diffViewAbortController = new AbortController();
  thunkAPI.signal = diffViewAbortController.signal;

  const state = thunkAPI.getState();
  const res = await renderDiff(
    {
      ...params,
      maxNodes: state.continuous.maxNodes,
    },
    diffViewAbortController
  );

  if (res.isOk) {
    return Promise.resolve({ profile: res.value });
  }

  if (res.isErr && res.error instanceof RequestAbortedError) {
    return thunkAPI.rejectWithValue({ rejectedWithValue: 'reloading' });
  }

  thunkAPI.dispatch(
    addNotification({
      type: 'danger',
      title: 'Failed to load diff view',
      message: res.error.message,
    })
  );

  return Promise.reject(res.error);
});
