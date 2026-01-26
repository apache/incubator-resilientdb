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

/* eslint-disable react/jsx-props-no-spreading */
import React, { useRef } from 'react';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import type { Units } from '@pyroscope/models/src';
import { DefaultPalette } from '../FlameGraph/FlameGraphComponent/colorPalette';

import TableTooltip, { TableTooltipProps } from './TableTooltip';

function TestTable(props: Omit<TableTooltipProps, 'tableBodyRef'>) {
  const tableBodyRef = useRef<HTMLTableSectionElement>(null);

  return (
    <>
      <table>
        <tbody data-testid="table-body" ref={tableBodyRef} />
      </table>
      <TableTooltip
        {...(props as TableTooltipProps)}
        tableBodyRef={tableBodyRef}
      />
    </>
  );
}

describe('TableTooltip', () => {
  const renderTable = (props: Omit<TableTooltipProps, 'tableBodyRef'>) =>
    render(<TestTable {...props} />);

  it('should render TableTooltip', () => {
    const props = {
      numTicks: 100,
      sampleRate: 100,
      units: 'samples' as Units,
      palette: DefaultPalette,
    };

    renderTable(props);

    userEvent.hover(screen.getByTestId('table-body'));

    expect(screen.getByTestId('tooltip')).toBeInTheDocument();
  });
});
