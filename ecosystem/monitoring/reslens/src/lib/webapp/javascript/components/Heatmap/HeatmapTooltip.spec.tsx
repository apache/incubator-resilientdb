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
import { render, screen } from '@testing-library/react';

import HeatmapTooltip from './HeatmapTooltip';
import { heatmapMockData } from '../../services/exemplarsTestData';

const canvasEl = document.createElement('canvas');
const canvasRef = { current: canvasEl } as RefObject<HTMLCanvasElement>;

describe('Component: HeatmapTooltip', () => {
  const renderTooltip = () => {
    render(
      <HeatmapTooltip
        dataSourceElRef={canvasRef}
        heatmapW={400}
        heatmap={heatmapMockData}
        timezone="browser"
        sampleRate={100}
      />
    );
  };

  it('should render initial tooltip (not active)', () => {
    renderTooltip();

    expect(screen.getByTestId('heatmap-tooltip')).toBeInTheDocument();
  });
});
