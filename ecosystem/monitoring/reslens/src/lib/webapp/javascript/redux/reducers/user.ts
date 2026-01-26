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

/* eslint-disable prettier/prettier */
import { createSlice } from '@reduxjs/toolkit';
import { type User } from '@webapp/models/users';
import { connect, useSelector } from 'react-redux';
import {
  loadCurrentUser as loadCurrentUserAPI,
  changeMyPassword as changeMyPasswordAPI,
  editMyUser as editMyUserAPI,
} from '@webapp/services/users';
import type { RootState } from '@webapp/redux/store';
import { addNotification } from './notifications';
import { createAsyncThunk } from '../async-thunk';

interface UserRootState {
  type: 'loading' | 'loaded' | 'failed';
  data?: User;
}

// Define the initial state using that type
const initialState: UserRootState = {
  type: 'loading',
  data: undefined,
};

export const loadCurrentUser = createAsyncThunk(
  'users/loadCurrentUser',
  async (_, thunkAPI) => {
    const res = await loadCurrentUserAPI();
    if (res.isOk) {
      return Promise.resolve(res.value);
    }

    // Suppress 401 error on login screen
    // TODO(petethepig): we need a better way of handling this exception
    if ('code' in res.error && res.error.code === 401) {
      return Promise.reject(res.error);
    }

    thunkAPI.dispatch(
      addNotification({
        type: 'danger',
        title: 'Failed to load current user',
        message: 'Please contact your administrator',
      })
    );

    return Promise.reject(res.error);
  }
);

const userSlice = createSlice({
  name: 'user',
  initialState,
  reducers: {},
  extraReducers: (builder) => {
    builder.addCase(loadCurrentUser.fulfilled, (state, action) => {
      return { type: 'loaded', data: action.payload };
    });
    builder.addCase(loadCurrentUser.pending, (state) => {
      return { type: 'loading', data: state.data };
    });
    builder.addCase(loadCurrentUser.rejected, (state) => {
      return { type: 'failed', data: state.data };
    });
  },
});

export const changeMyPassword = createAsyncThunk(
  'users/changeMyPassword',
  async (passwords: { oldPassword: string; newPassword: string }, thunkAPI) => {
    const res = await changeMyPasswordAPI(
      passwords.oldPassword,
      passwords.newPassword
    );

    if (res.isOk) {
      return Promise.resolve(true);
    }

    thunkAPI.dispatch(
      addNotification({
        type: 'danger',
        title: 'Failed',
        message: 'Failed to change users password',
      })
    );
    return thunkAPI.rejectWithValue(res.error);
  }
);

export const editMe = createAsyncThunk(
  'users/editMyUser',
  async (data: Partial<User>, thunkAPI) => {
    const res = await editMyUserAPI(data);

    if (res.isOk) {
      await thunkAPI.dispatch(loadCurrentUser()).unwrap();
      return Promise.resolve(res.value);
    }

    thunkAPI.dispatch(
      addNotification({
        type: 'danger',
        title: 'Failed',
        message: 'Failed to edit current user',
      })
    );
    return thunkAPI.rejectWithValue(res.error);
  }
);

export const currentUserState = (state: RootState) => state.user;
export const selectCurrentUser = (state: RootState) => state.user?.data;

// TODO: @shaleynikov extract currentUser HOC
// TODO(eh-am): get rid of HOC
export const withCurrentUser = (component: ShamefulAny) =>
  connect((state: RootState) => ({
    currentUser: selectCurrentUser(state),
  }))(function ConditionalRender(props: { currentUser: User }) {
    if (props.currentUser || !(window as ShamefulAny).isAuthRequired) {
      return component(props);
    }
    return null;
  } as ShamefulAny);

export const useCurrentUser = () => useSelector(selectCurrentUser);

export default userSlice.reducer;
