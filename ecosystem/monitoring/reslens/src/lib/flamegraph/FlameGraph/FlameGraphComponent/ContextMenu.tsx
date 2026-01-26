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

/* eslint-disable react/jsx-props-no-spreading, import/no-extraneous-dependencies */
import React from 'react';
import { ControlledMenu, useMenuState } from '../../../webapp/javascript/ui/Menu'
import styles from './ContextMenu.module.scss';

type xyToMenuItems = (x: number, y: number) => JSX.Element[];

export interface ContextMenuProps {
  canvasRef: React.RefObject<HTMLCanvasElement>;

  /**
   * The menu is built dynamically
   * Based on the cell's contents
   * only MenuItem and SubMenu should be supported
   */
  xyToMenuItems: xyToMenuItems;

  onClose: () => void;
  onOpen: (x: number, y: number) => void;
}

export default function ContextMenu(props: ContextMenuProps) {
  const [menuProps, toggleMenu] = useMenuState({ transition: true });
  const [anchorPoint, setAnchorPoint] = React.useState({ x: 0, y: 0 });
  const { canvasRef } = props;
  const [menuItems, setMenuItems] = React.useState<JSX.Element[]>([]);
  const {
    xyToMenuItems,
    onClose: onCloseCallback,
    onOpen: onOpenCallback,
  } = props;

  const onClose = () => {
    toggleMenu(false);

    onCloseCallback();
  };

  React.useEffect(() => {
    toggleMenu(false);

    // use closure to "cache" the current canvas reference
    // so that when cleaning up, it points to a valid canvas
    // (otherwise it would be null)
    const canvasEl = canvasRef.current;
    if (!canvasEl) {
      return () => {};
    }

    const onContextMenu = (e: MouseEvent) => {
      e.preventDefault();

      const items = xyToMenuItems(e.offsetX, e.offsetY);
      setMenuItems(items);

      // TODO
      // if the menu becomes too large, it may overflow to outside the screen
      const x = e.clientX;
      const y = e.clientY + 20;

      setAnchorPoint({ x, y });
      toggleMenu(true);

      onOpenCallback(e.offsetX, e.offsetY);
    };

    // watch for mouse events on the bar
    canvasEl.addEventListener('contextmenu', onContextMenu);

    return () => {
      canvasEl.removeEventListener('contextmenu', onContextMenu);
    };
  }, [xyToMenuItems]);

  return (
    <ControlledMenu
      {...menuProps}
      className={styles.dummy}
      anchorPoint={anchorPoint}
      onClose={onClose}
    >
      {menuItems}
    </ControlledMenu>
  );
}
