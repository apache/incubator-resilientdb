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

import { brandQuery, Query, queryToAppName } from '@webapp/models/query';
import type { RootState } from '@webapp/redux/store';
import { TagsState } from './state';

export const selectContinuousState = (state: RootState) => state.continuous;
export const selectApplicationName = (state: RootState) => {
  const { query } = selectQueries(state);

  const appName = queryToAppName(query);

  return appName.map((q) => q.split('{')[0]).unwrapOrElse(() => '');
};

export const selectAppNamesState = (state: RootState) => state.continuous.apps;
export const selectAppNames = (state: RootState) => {
  return state.continuous.apps.data.map((a) => a.name).sort();
};

export const selectComparisonState = (state: RootState) =>
  state.continuous.comparisonView;

export const selectAppTags = (query?: Query) => (state: RootState) => {
  if (query) {
    const appName = queryToAppName(query);
    if (appName.isJust) {
      if (state.continuous.tags[appName.value]) {
        return state.continuous.tags[appName.value];
      }
    }
  }

  return {
    type: 'pristine',
    tags: {},
  } as TagsState;
};

export const selectTimelineSides = (state: RootState) => {
  return {
    left: state.continuous.leftTimeline,
    right: state.continuous.rightTimeline,
  };
};

export const selectTimelineSidesData = (state: RootState) => {
  return {
    left: state.continuous.leftTimeline.timeline,
    right: state.continuous.rightTimeline.timeline,
  };
};

export const selectQueries = (state: RootState) => {
  return {
    leftQuery: brandQuery(state.continuous.leftQuery || ''),
    rightQuery: brandQuery(state.continuous.rightQuery || ''),
    query: brandQuery(state.continuous.query),
  };
};

// TODO: accept a side (continuous / leftside)
export const selectAnnotationsOrDefault = (state: RootState) => {
  if ('annotations' in state.continuous.singleView) {
    return state.continuous.singleView.annotations;
  }
  return [];
};

export const selectRanges = (rootState: RootState) => {
  const state = rootState.continuous;

  return {
    left: {
      from: state.leftFrom,
      until: state.leftUntil,
    },
    right: {
      from: state.rightFrom,
      until: state.rightUntil,
    },
    regular: {
      from: state.from,
      until: state.until,
    },
  };
};
