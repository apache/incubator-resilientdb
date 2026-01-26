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

import { createSlice, createSelector, PayloadAction } from '@reduxjs/toolkit';
import { createMigrate } from 'redux-persist';
import storage from 'redux-persist/lib/storage';
import { PersistedState } from 'redux-persist/lib/types';
import type { RootState } from '@webapp/redux/store';

// Persistence Migrations
// See examples on https://github.com/rt2zz/redux-persist/blob/master/docs/migrations.md
export const migrations = {
  0: (state: PersistedState) => {
    if (!state) {
      return {} as PersistedState;
    }

    return { ...state };
  },
};

export const persistConfig = {
  key: 'pyroscope:ui',
  version: 0,
  storage,
  migrate: createMigrate(migrations, { debug: true }),
};

type SidebarState =
  // pristine means user hasn't interacted with it yet
  // so we default to certain heuristics (eg window size)
  | { state: 'pristine'; collapsed: true }
  | { state: 'pristine'; collapsed: false }

  // userInteracted means user has actively clicked on the button
  // so we should keep whatever state they've chosen
  | { state: 'userInteracted'; collapsed: true }
  | { state: 'userInteracted'; collapsed: false };

export interface UiState {
  sidebar: SidebarState;
  time: {
    offset: null | number;
  };
  colorMode: 'dark' | 'light';
}

const initialState: UiState = {
  sidebar: { state: 'pristine', collapsed: window.innerWidth < 1200 },
  time: {
    offset: null,
  },
  //  sidebar: { state: 'pristine' },
  colorMode: 'dark',
};

export const uiSlice = createSlice({
  name: 'ui',
  initialState,
  reducers: {
    recalculateSidebar: (state) => {
      if (state.sidebar.state === 'pristine') {
        state.sidebar.collapsed = window.innerWidth < 1200;
      }
    },
    collapseSidebar: (state) => {
      state.sidebar = { state: 'userInteracted', collapsed: true };
    },
    uncollapseSidebar: (state) => {
      state.sidebar = { state: 'userInteracted', collapsed: false };
    },
    changeTimeZoneOffset: (state, action) => {
      state.time.offset = action.payload;
    },
    setColorMode: (state, action: PayloadAction<'dark' | 'light'>) => {
      state.colorMode = action.payload;
    },
  },
});

const selectUiState = (state: RootState) => state.ui;

export const selectSidebarCollapsed = createSelector(selectUiState, (state) => {
  return state.sidebar.collapsed;
});

export const selectTimezoneOffset = createSelector(
  selectUiState,
  (state) => state.time.offset
);

export const selectAppColorMode = createSelector(
  selectUiState,
  (state) => state.colorMode
);

export const {
  collapseSidebar,
  uncollapseSidebar,
  recalculateSidebar,
  changeTimeZoneOffset,
  setColorMode,
} = uiSlice.actions;

export default uiSlice.reducer;
