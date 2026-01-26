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

/* eslint-disable import/prefer-default-export */
import {
  createAsyncThunk as libCreateAsyncThunk,
  SerializedError,
} from '@reduxjs/toolkit';

// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
export const createAsyncThunk: typeof libCreateAsyncThunk = (
  ...args: Parameters<typeof libCreateAsyncThunk>
) => {
  const [typePrefix, payloadCreator, options] = args;
  return libCreateAsyncThunk(typePrefix, payloadCreator, {
    ...options,
    // Return the error as is (without)
    // So that the components can use features like instanceof, and accessing other fields that would otherwise be ignored
    // https://github.com/reduxjs/redux-toolkit/blob/db0d7dc20939b62f8c59631cc030575b78642296/packages/toolkit/src/createAsyncThunk.ts#L94
    serializeError: (x) => x as SerializedError,
  });
};
