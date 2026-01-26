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
import {
  ClickEvent,
  Menu,
  MenuProps,
  MenuHeader,
  SubMenu as LibSubmenu,
  MenuItem as LibMenuItem,
  MenuButton as LibMenuButton,
  FocusableItem as LibFocusableItem,
  MenuGroup as LibMenuGroup,
} from '@szhsin/react-menu';

import cx from 'classnames';
import styles from './Dropdown.module.scss';

export interface DropdownProps {
  id?: string;

  /** Whether the button is disabled or not */
  disabled?: boolean;
  ['data-testid']?: string;
  className?: string;
  menuButtonClassName?: string;

  /** Dropdown label */
  label: string;

  /** Dropdown value*/
  value?: string;

  children?: JSX.Element[] | JSX.Element;

  /** Event that fires when an item is activated*/
  onItemClick?: (event: ClickEvent) => void;

  overflow?: MenuProps['overflow'];
  position?: MenuProps['position'];
  direction?: MenuProps['direction'];
  align?: MenuProps['align'];
  viewScroll?: MenuProps['viewScroll'];
  arrow?: MenuProps['arrow'];
  offsetX?: MenuProps['offsetX'];
  offsetY?: MenuProps['offsetY'];

  ariaLabel?: MenuProps['aria-label'];

  menuButton?: JSX.Element;
}

export default function Dropdown({
  id,
  children,
  className,
  disabled,
  value,
  label,
  onItemClick,
  overflow,
  position,
  direction,
  align,
  viewScroll,
  arrow,
  offsetX,
  offsetY,
  menuButtonClassName = '',
  ariaLabel,
  ...props
}: DropdownProps) {
  const menuButtonComponent = props.menuButton || (
    <MenuButton
      aria-label={ariaLabel}
      className={`${styles.dropdownMenuButton} ${menuButtonClassName}`}
      disabled={disabled}
      type="button"
    >
      {value || label}
    </MenuButton>
  );

  return (
    <Menu
      id={id}
      aria-label={ariaLabel}
      className={cx(className, styles.dropdownMenu)}
      data-testid={props['data-testid']}
      onItemClick={onItemClick}
      overflow={overflow}
      position={position}
      direction={direction}
      align={align}
      viewScroll={viewScroll}
      arrow={arrow}
      offsetX={offsetX}
      offsetY={offsetY}
      menuButton={menuButtonComponent}
    >
      <MenuHeader>{label}</MenuHeader>
      {children}
    </Menu>
  );
}

export const SubMenu = LibSubmenu;
export const MenuItem = LibMenuItem;
export const MenuButton = LibMenuButton as any;
export const FocusableItem = LibFocusableItem;
export const MenuGroup = LibMenuGroup;
