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
import Color from 'color';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';

import type { TimelineGroupData } from '@webapp/components/TimelineChart/TimelineChartWrapper';
import type { Group } from '@pyroscope/models/src';

import Legend from './Legend';

const groups = [
  {
    tagName: 'tag1',
    color: Color('red'),
    data: {} as Group,
  },
  {
    tagName: 'tag2',
    color: Color('green'),
    data: {} as Group,
  },
];

describe('Component: Legend', () => {
  const renderLegend = (
    groups: TimelineGroupData[],
    handler: (v: string) => void
  ) => {
    render(
      <Legend
        activeGroup="All"
        groups={groups}
        handleGroupByTagValueChange={handler}
      />
    );
  };

  it('renders tags and colors correctly', () => {
    renderLegend(groups, () => {});

    expect(screen.getByTestId('legend')).toBeInTheDocument();
    expect(screen.getAllByTestId('legend-item')).toHaveLength(2);
    expect(screen.getAllByTestId('legend-item-color')).toHaveLength(2);
    screen.getAllByTestId('legend-item-color').forEach((element, index) => {
      expect(element).toHaveStyle(
        `background-color: ${groups[index].color.toString()}`
      );
    });
  });

  it('calls handleGroupByTagValueChange correctly', () => {
    const handleGroupByTagValueChangeMock = jest.fn();
    renderLegend(groups, handleGroupByTagValueChangeMock);

    expect(screen.getAllByTestId('legend-item')).toHaveLength(2);
    screen.getAllByTestId('legend-item').forEach((element) => {
      userEvent.click(element);

      expect(handleGroupByTagValueChangeMock).toHaveBeenCalledWith(
        element.textContent
      );
    });
  });
});
