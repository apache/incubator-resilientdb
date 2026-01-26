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
import { MenuItem } from '@webapp/ui/Menu';
import userEvent from '@testing-library/user-event';

import ContextMenu, { ContextMenuProps } from './ContextMenu';

const { queryByRole, queryAllByRole, getByRole } = screen;

function TestCanvas(props: Omit<ContextMenuProps, 'canvasRef'>) {
  const canvasRef = React.useRef<HTMLCanvasElement>(null);

  return (
    <>
      <canvas data-testid="canvas" ref={canvasRef} />
      <ContextMenu data-testid="contextmenu" canvasRef={canvasRef} {...props} />
    </>
  );
}

describe('ContextMenu', () => {
  it('works', () => {
    let hasBeenClicked = false;

    const xyToMenuItems = () => {
      return [
        <MenuItem
          key="test"
          onClick={() => {
            hasBeenClicked = true;
          }}
        >
          Test
        </MenuItem>,
      ];
    };

    render(
      <TestCanvas
        xyToMenuItems={xyToMenuItems}
        onClose={() => {}}
        onOpen={() => {}}
      />
    );

    expect(queryByRole('menu')).not.toBeInTheDocument();

    // trigger a right click
    userEvent.click(screen.getByTestId('canvas'), { buttons: 2 });

    expect(queryByRole('menu')).toBeVisible();
    expect(queryAllByRole('menuitem')).toHaveLength(1);

    userEvent.click(getByRole('menuitem'));
    expect(hasBeenClicked).toBe(true);
  });

  it('shows different items depending on the clicked node', () => {
    const xyToMenuItems = jest.fn();

    render(
      <TestCanvas
        xyToMenuItems={xyToMenuItems}
        onClose={() => {}}
        onOpen={() => {}}
      />
    );

    expect(queryByRole('menu')).not.toBeInTheDocument();

    // trigger a right click
    xyToMenuItems.mockReturnValueOnce([<MenuItem key="1">1</MenuItem>]);
    userEvent.click(screen.getByTestId('canvas'), { buttons: 2 });
    expect(queryAllByRole('menuitem')).toHaveLength(1);

    xyToMenuItems.mockReturnValueOnce([
      <MenuItem key="1">1</MenuItem>,
      <MenuItem key="2">2</MenuItem>,
    ]);
    userEvent.click(screen.getByTestId('canvas'), { buttons: 2 });
    expect(queryAllByRole('menuitem')).toHaveLength(2);
  });
});
