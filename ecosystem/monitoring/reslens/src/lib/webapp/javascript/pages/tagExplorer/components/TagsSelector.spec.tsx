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
import { Provider } from 'react-redux';
import { render, screen } from '@testing-library/react';
import { configureStore } from '@reduxjs/toolkit';
import { BrowserRouter } from 'react-router-dom';

import continuousReducer from '@webapp/redux/reducers/continuous';
import TagsSelector, { TagSelectorProps } from './TagsSelector';

const whereDropdownItems = ['foo', 'bar', 'baz'];
const groupByTag = 'group-by-tag-test';
const appName = 'app-name-test';
const linkName = 'link-name-test';

function createStore(preloadedState: any) {
  const store = configureStore({
    reducer: {
      continuous: continuousReducer,
    },
    preloadedState,
  });

  return store;
}

describe('Component: ViewTagsSelectLinkModal', () => {
  const renderComponent = (props: TagSelectorProps) => {
    render(
      <Provider
        store={createStore({
          continuous: {},
        })}
      >
        <TagsSelector {...props} />
      </Provider>,
      // https://github.com/testing-library/react-testing-library/issues/972
      { wrapper: BrowserRouter as ShamefulAny }
    );
  };

  it('shoudld successfully render ModalWithToggle', () => {
    renderComponent({
      appName,
      groupByTag,
      linkName,
      whereDropdownItems,
    });

    // triggers click
    screen.getByTestId('toggler').click();
    const modalWithToggleEl = screen.getByTestId('modal');

    expect(modalWithToggleEl).toBeInTheDocument();

    // static
    expect(
      screen.getByText('Select Tags For link-name-test')
    ).toBeInTheDocument();
    expect(screen.getByText('baseline')).toBeInTheDocument();
    expect(screen.getByText('comparison')).toBeInTheDocument();
    expect(
      modalWithToggleEl.querySelector('.modalFooter input')
    ).toBeInTheDocument();
    expect(
      modalWithToggleEl.querySelector('.modalFooter input')
    ).toHaveAttribute('value', 'Compare tags');

    // dynamic
    expect(modalWithToggleEl.querySelectorAll('.tags')).toHaveLength(2);
    modalWithToggleEl.querySelectorAll('.tags').forEach((tagList) => {
      tagList.querySelectorAll('input').forEach((tag, i) => {
        expect(tag).toHaveAttribute('value', whereDropdownItems[i]);
        tag.click();
        expect(tag.parentElement).toHaveClass('selected');
      });
    });

    //second click
    screen.getByTestId('toggler').click();
    expect(modalWithToggleEl).not.toBeInTheDocument();
  });
});
