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

import { Query, queryToAppName } from '@webapp/models/query';
import * as tagsService from '@webapp/services/tags';
import { createBiggestInterval } from '@webapp/util/timerange';
import { formatAsOBject, toUnixTimestamp } from '@webapp/util/formatDate';
import { ContinuousState } from './state';
import { addNotification } from '../notifications';
import { createAsyncThunk } from '../../async-thunk';

function biggestTimeRangeInUnix(state: ContinuousState) {
  return createBiggestInterval({
    from: [state.from, state.leftFrom, state.rightFrom]
      .map(formatAsOBject)
      .map(toUnixTimestamp),
    until: [state.until, state.leftUntil, state.leftUntil]
      .map(formatAsOBject)
      .map(toUnixTimestamp),
  });
}

function assertIsValidAppName(query: Query) {
  const appName = queryToAppName(query);
  if (appName.isNothing) {
    throw Error(`Query '${query}' is not a valid app`);
  }

  return appName.value;
}

export const fetchTags = createAsyncThunk<
  { appName: string; tags: string[]; from: number; until: number },
  Query,
  { state: { continuous: ContinuousState } }
>(
  'continuous/fetchTags',
  async (query: Query, thunkAPI) => {
    const appName = assertIsValidAppName(query);

    const state = thunkAPI.getState().continuous;
    const timerange = biggestTimeRangeInUnix(state);
    const res = await tagsService.fetchTags(
      query,
      timerange.from,
      timerange.until
    );

    if (res.isOk) {
      return Promise.resolve({
        appName,
        tags: res.value,
        from: timerange.from,
        until: timerange.until,
      });
    }

    thunkAPI.dispatch(
      addNotification({
        type: 'danger',
        title: 'Failed to load tags',
        message: res.error.message,
      })
    );

    return Promise.reject(res.error);
  },
  {
    // If we already loaded the tags for that application
    // And we are trying to load tags for a smaller range
    // Skip it, since we most likely already have that data
    condition: (query, thunkAPI) => {
      const appName = assertIsValidAppName(query);
      const state = thunkAPI.getState().continuous;
      const timerange = biggestTimeRangeInUnix(state);

      const s = state.tags[appName];

      // Haven't loaded yet
      if (!s) {
        return true;
      }

      // Already loading that tag
      if (s.type === 'loading') {
        return false;
      }

      // Any other state that's not loaded
      if (s.type !== 'loaded') {
        return true;
      }

      const isInRange = (target: number) => {
        return target >= s.from && target <= s.until;
      };

      const isSmallerThanLoaded =
        isInRange(timerange.from) && isInRange(timerange.until);

      return !isSmallerThanLoaded;
    },
  }
);
export const fetchTagValues = createAsyncThunk<
  {
    appName: string;
    label: string;
    values: string[];
  },
  {
    query: Query;
    label: string;
  },
  { state: { continuous: ContinuousState } }
>(
  'continuous/fetchTagsValues',
  async (payload: { query: Query; label: string }, thunkAPI) => {
    const appName = assertIsValidAppName(payload.query);

    const state = thunkAPI.getState().continuous.tags[appName];
    if (!state || state.type !== 'loaded') {
      return Promise.reject(
        new Error(
          `Trying to load label-values for an unloaded label. This is likely due to a race condition.`
        )
      );
    }

    const res = await tagsService.fetchLabelValues(
      payload.label,
      payload.query,
      state.from,
      state.until
    );

    if (res.isOk) {
      return Promise.resolve({
        appName,
        label: payload.label,
        values: res.value,
      });
    }

    thunkAPI.dispatch(
      addNotification({
        type: 'danger',
        title: 'Failed to load tag values',
        message: res.error.message,
      })
    );

    return Promise.reject(res.error);
  },
  {
    condition: ({ query, label }, thunkAPI) => {
      const appName = assertIsValidAppName(query);

      // Are we trying to load values from a tag that wasn't loaded?
      // If so it's most likely due to a race condition
      const tagState = thunkAPI.getState().continuous.tags[appName];
      if (!tagState || tagState.type !== 'loaded') {
        return false;
      }

      const tagValueState = tagState.tags[label];
      // Have not being loaded yet
      if (!tagValueState) {
        return true;
      }

      // Loading or already loaded
      if (tagValueState.type === 'loading' || tagValueState.type === 'loaded') {
        return false;
      }

      return true;
    },
  }
);
