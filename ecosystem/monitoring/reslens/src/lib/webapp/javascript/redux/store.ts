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
  persistStore,
  persistReducer,
  FLUSH,
  REHYDRATE,
  PAUSE,
  PERSIST,
  PURGE,
  REGISTER,
} from 'redux-persist';

// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore: Until we rewrite FlamegraphRenderer in typescript this will do
import ReduxQuerySync from 'redux-query-sync';
import { configureStore, combineReducers, Middleware } from '@reduxjs/toolkit';

import history from '../util/history';

import settingsReducer from './reducers/settings';
import userReducer from './reducers/user';
import continuousReducer, {
  actions as continuousActions,
} from './reducers/continuous';
import tracingReducer, { actions as tracingActions } from './reducers/tracing';
import serviceDiscoveryReducer from './reducers/serviceDiscovery';
import adhocReducer from './reducers/adhoc';
import uiStore, { persistConfig as uiPersistConfig } from './reducers/ui';

const reducer = combineReducers({
  settings: settingsReducer,
  user: userReducer,
  serviceDiscovery: serviceDiscoveryReducer,
  ui: persistReducer(uiPersistConfig, uiStore),
  continuous: continuousReducer,
  tracing: tracingReducer,
  adhoc: adhocReducer,
});

// Most times we will display a (somewhat) user friendly message toast
// But it's still useful to have the actual error logged to the console
export const logErrorMiddleware: Middleware = () => (next) => (action) => {
  next(action);
  if (action?.error) {
    console.error(action.error);
  }
};

const store = configureStore({
  reducer,
  // https://github.com/reduxjs/redux-toolkit/issues/587#issuecomment-824927971
  middleware: (getDefaultMiddleware) =>
    getDefaultMiddleware({
      serializableCheck: {
        ignoredActionPaths: ['error'],

        // Based on this issue: https://github.com/rt2zz/redux-persist/issues/988
        // and this guide https://redux-toolkit.js.org/usage/usage-guide#use-with-redux-persist
        ignoredActions: [
          FLUSH,
          REHYDRATE,
          PAUSE,
          PERSIST,
          PURGE,
          REGISTER,
          'adhoc/uploadFile/pending',
          'adhoc/uploadFile/fulfilled',
        ],
      },
    }).concat([logErrorMiddleware]),
});

export const persistor = persistStore(store);

// This is a bi-directional sync between the query parameters and the redux store
// It works as follows:
// * When URL query changes, It will dispatch the action
// * When the store changes (the field set in selector), the query param is updated
// For more info see the implementation at
// https://github.com/Treora/redux-query-sync/blob/master/src/redux-query-sync.js
ReduxQuerySync({
  store,
  params: {
    from: {
      defaultValue: 'now-1h',
      selector: (state: RootState) => state.continuous.from,
      action: continuousActions.setFrom,
    },
    until: {
      defaultValue: 'now',
      selector: (state: RootState) => state.continuous.until,
      action: continuousActions.setUntil,
    },
    leftFrom: {
      defaultValue: 'now-1h',
      selector: (state: RootState) => state.continuous.leftFrom,
      action: continuousActions.setLeftFrom,
    },
    leftUntil: {
      defaultValue: 'now-30m',
      selector: (state: RootState) => state.continuous.leftUntil,
      action: continuousActions.setLeftUntil,
    },
    rightFrom: {
      defaultValue: 'now-30m',
      selector: (state: RootState) => state.continuous.rightFrom,
      action: continuousActions.setRightFrom,
    },
    rightUntil: {
      defaultValue: 'now',
      selector: (state: RootState) => state.continuous.rightUntil,
      action: continuousActions.setRightUntil,
    },
    query: {
      defaultvalue: '',
      selector: (state: RootState) => state.continuous.query,
      action: continuousActions.setQuery,
    },
    queryID: {
      defaultvalue: '',
      selector: (state: RootState) => state.tracing.queryID,
      action: tracingActions.setQueryID,
    },
    rightQuery: {
      defaultvalue: '',
      selector: (state: RootState) => state.continuous.rightQuery,
      action: continuousActions.setRightQuery,
    },
    leftQuery: {
      defaultvalue: '',
      selector: (state: RootState) => state.continuous.leftQuery,
      action: continuousActions.setLeftQuery,
    },
    maxNodes: {
      defaultValue: '0',
      selector: (state: RootState) => state.continuous.maxNodes,
      action: continuousActions.setMaxNodes,
    },
    groupBy: {
      defaultValue: '',
      selector: (state: RootState) =>
        state.continuous.tagExplorerView.groupByTag,
      action: continuousActions.setTagExplorerViewGroupByTag,
    },
    groupByValue: {
      defaultValue: '',
      selector: (state: RootState) =>
        state.continuous.tagExplorerView.groupByTagValue,
      action: continuousActions.setTagExplorerViewGroupByTagValue,
    },
  },
  initialTruth: 'location',
  replaceState: false,
  history,
});
export default store;

// Infer the `RootState` and `AppDispatch` types from the store itself
export type RootState = ReturnType<typeof store.getState>;
// Inferred type: {posts: PostsState, comments: CommentsState, users: UsersState}
export type AppDispatch = typeof store.dispatch;
