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

import React from 'react';
import { MemoryRouter } from 'react-router-dom';
import { render, screen } from '@testing-library/react';
import { configureStore } from '@reduxjs/toolkit';
import { Provider } from 'react-redux';
import uiReducer from '@webapp/redux/reducers/ui';

import { SidebarComponent } from './Sidebar';

// TODO: figure out the types here
function createStore(preloadedState: any) {
  const store = configureStore({
    reducer: {
      ui: uiReducer,
    },
    preloadedState,
  });

  return store;
}

describe('Sidebar', () => {
  describe('active routes highlight', () => {
    describe.each([
      ['/', 'sidebar-continuous-single'],
      ['/comparison', 'sidebar-continuous-comparison'],
      ['/comparison-diff', 'sidebar-continuous-diff'],
    ])('visiting route %s', (a, b) => {
      describe('collapsed', () => {
        test(`should have menuitem ${b} active`, () => {
          render(
            <MemoryRouter initialEntries={[a]}>
              <Provider
                store={createStore({
                  ui: {
                    sidebar: {
                      state: 'pristine',
                      collapsed: true,
                    },
                  },
                })}
              >
                <SidebarComponent />
              </Provider>
            </MemoryRouter>
          );

          // it should be active
          expect(screen.getByTestId(b)).toHaveClass('active');
        });
      });

      describe('not collapsed', () => {
        test(`should have menuitem ${b} active`, () => {
          render(
            <MemoryRouter initialEntries={[a]}>
              <Provider
                store={createStore({
                  ui: {
                    sidebar: {
                      state: 'pristine',
                      collapsed: false,
                    },
                  },
                })}
              >
                <SidebarComponent />
              </Provider>
            </MemoryRouter>
          );

          // it should be active
          expect(screen.getByTestId(b)).toHaveClass('active');
        });
      });
    });
  });
});
