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
import React from 'react';
import { render, screen } from '@testing-library/react';
import userEvent from '@testing-library/user-event';
import { Maybe } from 'true-myth';

import Highlight, { HighlightProps } from './Highlight';

function TestComponent(props: Omit<HighlightProps, 'canvasRef'>) {
  const canvasRef = React.useRef<HTMLCanvasElement>(null);

  return (
    <>
      <canvas data-testid="canvas" ref={canvasRef} />
      {canvasRef && <Highlight canvasRef={canvasRef} {...props} />}
    </>
  );
}

describe('Highlight', () => {
  it('works', () => {
    const xyToHighlightData = jest.fn();
    render(
      <TestComponent
        barHeight={50}
        xyToHighlightData={xyToHighlightData}
        zoom={Maybe.nothing()}
      />
    );

    // hover over a bar
    xyToHighlightData.mockReturnValueOnce(
      Maybe.of({
        left: 10,
        top: 5,
        width: 100,
      })
    );
    userEvent.hover(screen.getByTestId('canvas'));
    expect(screen.getByTestId('flamegraph-highlight')).toBeVisible();
    expect(screen.getByTestId('flamegraph-highlight')).toHaveStyle({
      height: '50px',
      left: '10px',
      top: '5px',
      width: '100px',
    });

    // hover outside the canvas
    xyToHighlightData.mockReturnValueOnce(Maybe.nothing());
    userEvent.hover(screen.getByTestId('canvas'));
    expect(screen.getByTestId('flamegraph-highlight')).not.toBeVisible();
  });
});
