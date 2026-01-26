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

import {
  RenderExploreOutput,
  renderSingle,
  RenderOutput,
  renderExplore,
} from '@webapp/services/render';
import { RequestAbortedError } from '@webapp/services/base';
import { appendLabelToQuery } from '@webapp/util/query';
import { addNotification } from '../notifications';
import { createAsyncThunk } from '../../async-thunk';
import { ContinuousState } from './state';

export const ALL_TAGS = 'All';

let tagExplorerViewAbortController: AbortController | undefined;
let tagExplorerViewProfileAbortController: AbortController | undefined;

export const fetchTagExplorerView = createAsyncThunk<
  RenderExploreOutput,
  null,
  { state: { continuous: ContinuousState } }
>('continuous/tagExplorerView', async (_, thunkAPI) => {
  if (tagExplorerViewAbortController) {
    tagExplorerViewAbortController.abort();
  }

  tagExplorerViewAbortController = new AbortController();
  thunkAPI.signal = tagExplorerViewAbortController.signal;

  const state = thunkAPI.getState();
  const res = await renderExplore(
    {
      query: state.continuous.query,
      from: state.continuous.from,
      until: state.continuous.until,
      groupBy: state.continuous.tagExplorerView.groupByTag,
      grouByTagValue: state.continuous.tagExplorerView.groupByTagValue,
      refreshToken: state.continuous.refreshToken,
    },
    tagExplorerViewAbortController
  );

  if (res.isOk) {
    return Promise.resolve(res.value);
  }

  if (res.isErr && res.error instanceof RequestAbortedError) {
    return Promise.reject(res.error);
  }

  thunkAPI.dispatch(
    addNotification({
      type: 'danger',
      title: 'Failed to load explore view data',
      message: res.error.message,
    })
  );

  return Promise.reject(res.error);
});

export const fetchTagExplorerViewProfile = createAsyncThunk<
  RenderOutput,
  null,
  { state: { continuous: ContinuousState } }
>('continuous/fetchTagExplorerViewProfile', async (_, thunkAPI) => {
  if (tagExplorerViewProfileAbortController) {
    tagExplorerViewProfileAbortController.abort();
  }

  tagExplorerViewProfileAbortController = new AbortController();
  thunkAPI.signal = tagExplorerViewProfileAbortController.signal;

  const state = thunkAPI.getState();
  const { groupByTag, groupByTagValue } = state.continuous.tagExplorerView;
  // if "All" option is selected we dont need to modify query to fetch profile
  const queryProps =
    ALL_TAGS === groupByTagValue
      ? { groupBy: groupByTag, query: state.continuous.query }
      : {
          query: appendLabelToQuery(
            state.continuous.query,
            state.continuous.tagExplorerView.groupByTag,
            state.continuous.tagExplorerView.groupByTagValue
          ),
        };
  const res = await renderSingle(
    {
      ...state.continuous,
      ...queryProps,
    },
    tagExplorerViewProfileAbortController
  );

  if (res.isOk) {
    return Promise.resolve(res.value);
  }

  if (res.isErr && res.error instanceof RequestAbortedError) {
    return Promise.reject(res.error);
  }

  thunkAPI.dispatch(
    addNotification({
      type: 'danger',
      title: 'Failed to load explore view profile',
      message: res.error.message,
    })
  );

  return Promise.reject(res.error);
});
