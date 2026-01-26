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
import { render, screen, within } from '@testing-library/react';

import { Heatmap } from '.';
import { heatmapMockData } from '../../services/exemplarsTestData';

jest.mock('./useHeatmapSelection.hook', () => ({
  ...jest.requireActual('./useHeatmapSelection.hook'),
  useHeatmapSelection: () => ({
    selectedCoordinates: { start: null, end: null },
    selectedAreaToHeatmapRatio: 1,
    hasSelectedArea: false,
  }),
}));

const renderHeatmap = () => {
  render(
    <Heatmap
      sampleRate={100}
      heatmap={heatmapMockData}
      onSelection={() => ({})}
      timezone="utc"
    />
  );
};

describe('Component: Heatmap', () => {
  it('should have all main elements', () => {
    renderHeatmap();

    expect(screen.getByTestId('heatmap-container')).toBeInTheDocument();
    expect(screen.getByTestId('y-axis')).toBeInTheDocument();
    expect(screen.getByTestId('x-axis')).toBeInTheDocument();
    expect(screen.getByRole('img')).toBeInTheDocument();
    expect(screen.getByTestId('selection-canvas')).toBeInTheDocument();
    expect(screen.getByTestId('color-scale')).toBeInTheDocument();
  });

  it('should have correct x-axis', () => {
    renderHeatmap();

    const xAxisTicks = within(screen.getByTestId('x-axis')).getAllByRole(
      'textbox'
    );
    expect(xAxisTicks).toHaveLength(8);
  });

  it('should have correct y-axis', () => {
    renderHeatmap();

    const xAxisTicks = within(screen.getByTestId('y-axis')).getAllByRole(
      'textbox'
    );
    expect(xAxisTicks).toHaveLength(6);
  });

  it('should have correct color scale', () => {
    renderHeatmap();

    const [maxTextEl, midTextEl, minTextEl] = within(
      screen.getByTestId('color-scale')
    ).getAllByRole('textbox');
    expect(maxTextEl.textContent).toBe(heatmapMockData.maxDepth.toString());
    expect(midTextEl.textContent).toBe('11539');
    expect(minTextEl.textContent).toBe(heatmapMockData.minDepth.toString());
  });
});
