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
import { brandQuery } from '@webapp/models/query';
import { render, screen, fireEvent } from '@testing-library/react';

import QueryInput from './QueryInput';

describe('QueryInput', () => {
  it('changes content correctly', () => {
    const onSubmit = jest.fn();
    render(
      <QueryInput initialQuery={brandQuery('myquery')} onSubmit={onSubmit} />
    );

    const form = screen.getByRole('form', { name: /query-input/i });
    fireEvent.submit(form);
    expect(onSubmit).toHaveBeenCalledWith('myquery');

    const input = screen.getByRole('textbox');
    fireEvent.change(input, { target: { value: 'myquery2' } });
    fireEvent.submit(form);
    expect(onSubmit).toHaveBeenCalledWith('myquery2');
  });

  describe('submission', () => {
    const onSubmit = jest.fn();

    beforeEach(() => {
      render(
        <QueryInput initialQuery={brandQuery('myquery')} onSubmit={onSubmit} />
      );
    });

    it('is submitted by pressing Enter', () => {
      const input = screen.getByRole('textbox');
      fireEvent.keyDown(input, { key: 'Enter' });
      expect(onSubmit).toHaveBeenCalledWith('myquery');
    });

    it('is submitted by clicking on the Execute button', () => {
      const button = screen.getByRole('button');
      button.click();
      expect(onSubmit).toHaveBeenCalledWith('myquery');
    });
  });
});
