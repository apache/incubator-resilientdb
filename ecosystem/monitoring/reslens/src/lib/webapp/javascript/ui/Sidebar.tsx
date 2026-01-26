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
import {
  ProSidebar,
  Menu as RProMenu,
  MenuItem as RProMenuItem,
  SubMenu as RProSubMenu,
  SidebarFooter as RProFooter,
  SidebarHeader as RProHeader,
  SidebarContent as RProContent,
  MenuItemProps,
  SubMenuProps,
} from 'react-pro-sidebar';
import { IconProps } from '@webapp/ui/Icon';
import './Sidebar.scss';

export interface SidebarProps {
  children: React.ReactNode | React.ReactNode[];

  collapsed?: boolean;
  className?: string;
}

// Sidebar is an abstraction over react-pro-sidebar
// So that we can some day reimplement its functinoality ourselves
export default function Sidebar(props: SidebarProps) {
  const { children, collapsed, className } = props;

  return (
    <ProSidebar className={className} collapsed={collapsed}>
      {children}
    </ProSidebar>
  );
}

// type check to validate only our Icon component can be used
type Icon = React.ReactElement<IconProps>;

export function MenuItem(props: MenuItemProps & { icon: Icon }) {
  const { className } = props;

  return <RProMenuItem {...props} className={className} />;
}

export function SubMenu(
  props: SubMenuProps & { active?: boolean; icon: Icon }
) {
  let { popperarrow, className } = props;
  // remove active since underlying component does not use it
  const { active, ...newProps } = props;

  if (popperarrow === undefined) {
    // set arrow between element and menu when collapsed by default, since that makes ux better
    popperarrow = true;
  }

  if (active) {
    if (!className) {
      className = '';
    }

    className += ' active';
  }

  return (
    <RProSubMenu
      {...newProps}
      popperarrow={popperarrow}
      className={className}
    />
  );
}

// Re-export the type so that end users only interact with our abstraction
export const Menu = RProMenu;
export const SidebarHeader = RProHeader;
export const SidebarFooter = RProFooter;
export const SidebarContent = RProContent;
