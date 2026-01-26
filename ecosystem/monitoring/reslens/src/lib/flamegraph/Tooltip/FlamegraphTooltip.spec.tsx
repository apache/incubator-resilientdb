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
import { Maybe } from 'true-myth';
import type { Units } from '@pyroscope/models/src';

import FlamegraphTooltip, { FlamegraphTooltipProps } from './FlamegraphTooltip';
import { DefaultPalette } from '../';

function TestCanvas(props: Omit<FlamegraphTooltipProps, 'canvasRef'>) {
  const canvasRef = useRef<HTMLCanvasElement>(null);

  return (
    <>
      <canvas data-testid="canvas" ref={canvasRef} />
      <FlamegraphTooltip
        {...(props as FlamegraphTooltipProps)}
        canvasRef={canvasRef}
      />
    </>
  );
}

describe('FlamegraphTooltip', () => {
  const renderCanvas = (
    props: Omit<FlamegraphTooltipProps, 'canvasRef' | 'palette'>
  ) => render(<TestCanvas {...props} palette={DefaultPalette} />);

  it('should render FlamegraphTooltip with single format', () => {
    const xyToData = (x: number, y: number) =>
      Maybe.of({
        format: 'single' as const,
        name: 'function_title',
        total: 10,
      });

    const props = {
      numTicks: 100,
      sampleRate: 100,
      xyToData,
      leftTicks: 100,
      rightTicks: 100,
      format: 'single' as const,
      units: 'samples' as Units,
    };

    renderCanvas(props);

    userEvent.hover(screen.getByTestId('canvas'));

    expect(screen.getByTestId('tooltip')).toBeInTheDocument();
  });

  it('should render FlamegraphTooltip with double format', () => {
    const xyToData = (x: number, y: number) =>
      Maybe.of({
        format: 'double' as const,
        name: 'my_function',
        totalLeft: 100,
        totalRight: 0,
        barTotal: 100,
      });

    const props = {
      numTicks: 100,
      sampleRate: 100,
      xyToData,
      leftTicks: 1000,
      rightTicks: 1000,
      format: 'double' as const,
      units: 'samples' as Units,
    };

    renderCanvas(props);

    userEvent.hover(screen.getByTestId('canvas'));

    expect(screen.getByTestId('tooltip')).toBeInTheDocument();
  });
});
