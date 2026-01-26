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

import React, { RefObject } from 'react';
import { renderHook } from '@testing-library/react-hooks';
import { Provider } from 'react-redux';
import { configureStore } from '@reduxjs/toolkit';
import continuousReducer from '@webapp/redux/reducers/continuous';
import tracingReducer from '@webapp/redux/reducers/tracing';

import { useHeatmapSelection } from './useHeatmapSelection.hook';
import { heatmapMockData } from '../../services/exemplarsTestData';

const canvasEl = document.createElement('canvas');
const divEl = document.createElement('div');
const canvasRef = { current: canvasEl } as RefObject<HTMLCanvasElement>;
const resizedSelectedAreaRef = { current: divEl } as RefObject<HTMLDivElement>;

function createStore(preloadedState: any) {
  const store = configureStore({
    reducer: {
      continuous: continuousReducer,
      tracing: tracingReducer,
    },
    preloadedState,
  });

  return store;
}

describe('Hook: useHeatmapSelection', () => {
  const render = () =>
    renderHook(
      () =>
        useHeatmapSelection({
          canvasRef,
          resizedSelectedAreaRef,
          heatmapW: 1234,
          heatmap: heatmapMockData,
          onSelection: () => ({}),
        }),
      {
        wrapper: ({ children }) => (
          <Provider
            store={createStore({
              continuous: {},
              tracing: {
                exemplarsSingleView: {},
              },
            })}
          >
            {children}
          </Provider>
        ),
      }
    ).result;

  it('should return initial selection values', () => {
    const { current } = render();

    expect(current).toMatchObject({
      selectedCoordinates: { start: null, end: null },
      selectedAreaToHeatmapRatio: 1,
      resetSelection: expect.any(Function),
    });
  });
});
