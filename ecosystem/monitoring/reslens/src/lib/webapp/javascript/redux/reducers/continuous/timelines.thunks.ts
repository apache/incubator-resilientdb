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

import { renderSingle } from '@webapp/services/render';
import type { Timeline } from '@webapp/models/timeline';
import { RequestAbortedError } from '@webapp/services/base';
import { addNotification } from '../notifications';
import { createAsyncThunk } from '../../async-thunk';
import { ContinuousState } from './state';

let sideTimelinesAbortController: AbortController | undefined;

export const fetchSideTimelines = createAsyncThunk<
  { left: Timeline; right: Timeline },
  null,
  { state: { continuous: ContinuousState } }
>('continuous/fetchSideTimelines', async (_, thunkAPI) => {
  if (sideTimelinesAbortController) {
    sideTimelinesAbortController.abort();
  }

  sideTimelinesAbortController = new AbortController();
  thunkAPI.signal = sideTimelinesAbortController.signal;

  const state = thunkAPI.getState();

  const res = await Promise.all([
    renderSingle(
      {
        query: state.continuous.leftQuery || '',
        from: state.continuous.from,
        until: state.continuous.until,
        maxNodes: state.continuous.maxNodes,
        refreshToken: state.continuous.refreshToken,
      },
      sideTimelinesAbortController
    ),
    renderSingle(
      {
        query: state.continuous.rightQuery || '',
        from: state.continuous.from,
        until: state.continuous.until,
        maxNodes: state.continuous.maxNodes,
        refreshToken: state.continuous.refreshToken,
      },
      sideTimelinesAbortController
    ),
  ]);

  if (
    (res?.[0]?.isErr && res?.[0]?.error instanceof RequestAbortedError) ||
    (res?.[1]?.isErr && res?.[1]?.error instanceof RequestAbortedError) ||
    (!res && thunkAPI.signal.aborted)
  ) {
    return Promise.reject();
  }

  if (res?.[0].isOk && res?.[1].isOk) {
    return Promise.resolve({
      left: res[0].value.timeline,
      right: res[1].value.timeline,
    });
  }

  thunkAPI.dispatch(
    addNotification({
      type: 'danger',
      title: `Failed to load the timelines`,
      message: '',
      additionalInfo: [
        res?.[0].error.message,
        res?.[1].error.message,
      ] as string[],
    })
  );

  return Promise.reject(res && res.filter((a) => a?.isErr).map((a) => a.error));
});
