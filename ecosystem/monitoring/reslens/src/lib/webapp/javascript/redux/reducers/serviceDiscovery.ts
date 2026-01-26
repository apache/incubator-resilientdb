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

import { Target } from '@webapp/models/targets';
import { fetchTargets } from '@webapp/services/serviceDiscovery';
import { createSlice } from '@reduxjs/toolkit';
import { addNotification } from './notifications';
import type { RootState } from '../store';
import { createAsyncThunk } from '../async-thunk';

export const loadTargets = createAsyncThunk(
  'serviceDiscovery/loadTargets',
  async (_, thunkAPI) => {
    const res = await fetchTargets();

    if (res.isOk) {
      return Promise.resolve(res.value);
    }

    thunkAPI.dispatch(
      addNotification({
        type: 'danger',
        title: 'Failed to load targets',
        message: res.error.message,
      })
    );

    return Promise.reject(res.error);
  }
);

interface State {
  type: 'pristine' | 'loading' | 'failed' | 'loaded';
  data: Target[];
}
const initialState: State = { type: 'loaded', data: [] };

export const serviceDiscoverySlice = createSlice({
  name: 'serviceDiscovery',
  initialState,
  reducers: {},
  extraReducers: (builder) => {
    builder.addCase(loadTargets.fulfilled, (state, action) => {
      state.data = action.payload;
      state.type = 'loaded';
    });

    builder.addCase(loadTargets.pending, (state) => {
      state.type = 'loading';
    });
    builder.addCase(loadTargets.rejected, (state) => {
      state.type = 'failed';
    });
  },
});

export default serviceDiscoverySlice.reducer;

export function selectTargetsData(s: RootState) {
  return s.serviceDiscovery.data;
}
