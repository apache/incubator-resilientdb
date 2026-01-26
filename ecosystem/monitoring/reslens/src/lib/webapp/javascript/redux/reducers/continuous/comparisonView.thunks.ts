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

import { renderSingle, RenderOutput } from '@webapp/services/render';
import { RequestAbortedError } from '@webapp/services/base';
import { addNotification } from '../notifications';
import { createAsyncThunk } from '../../async-thunk';
import { ContinuousState } from './state';

let comparisonSideAbortControllerLeft: AbortController | undefined;
let comparisonSideAbortControllerRight: AbortController | undefined;

export const fetchComparisonSide = createAsyncThunk<
  { side: 'left' | 'right'; data: Pick<RenderOutput, 'profile'> },
  { side: 'left' | 'right'; query: string },
  { state: { continuous: ContinuousState } }
>('continuous/fetchComparisonSide', async ({ side, query }, thunkAPI) => {
  const state = thunkAPI.getState();

  const res = await (() => {
    switch (side) {
      case 'left': {
        if (comparisonSideAbortControllerLeft) {
          comparisonSideAbortControllerLeft.abort();
        }

        comparisonSideAbortControllerLeft = new AbortController();
        thunkAPI.signal = comparisonSideAbortControllerLeft.signal;

        return renderSingle(
          {
            ...state.continuous,
            query,

            from: state.continuous.leftFrom,
            until: state.continuous.leftUntil,
          },
          comparisonSideAbortControllerLeft
        );
      }
      case 'right': {
        if (comparisonSideAbortControllerRight) {
          comparisonSideAbortControllerRight.abort();
        }

        comparisonSideAbortControllerRight = new AbortController();
        thunkAPI.signal = comparisonSideAbortControllerRight.signal;

        return renderSingle(
          {
            ...state.continuous,
            query,

            from: state.continuous.rightFrom,
            until: state.continuous.rightUntil,
          },
          comparisonSideAbortControllerRight
        );
      }
      default: {
        throw new Error('invalid side');
      }
    }
  })();

  if (res?.isErr && res?.error instanceof RequestAbortedError) {
    return thunkAPI.rejectWithValue({ rejectedWithValue: 'reloading' });
  }

  if (res.isOk) {
    return Promise.resolve({
      side,
      data: {
        profile: res.value.profile,
      },
    });
  }

  thunkAPI.dispatch(
    addNotification({
      type: 'danger',
      title: `Failed to load the ${side} side comparison`,
      message: res.error.message,
    })
  );

  return Promise.reject(res.error);
});
