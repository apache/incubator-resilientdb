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
  Tabs as MuiTabs,
  Tab as MuiTab,
  TabsProps,
  TabProps,
} from '@mui/material';
import styles from './Tabs.module.scss';

interface TabPanelProps {
  index: number;
  value: number;
  children: React.ReactNode;
}

export function Tabs({ children, value, onChange }: TabsProps) {
  return (
    <MuiTabs
      TabIndicatorProps={{
        hidden: true, // hide indicator
      }}
      className={styles.tabs}
      value={value}
      onChange={onChange}
    >
      {children}
    </MuiTabs>
  );
}

export function Tab({ label, ...rest }: TabProps) {
  return (
    <MuiTab disableRipple className={styles.tab} {...rest} label={label} />
  );
}

export function TabPanel({ children, value, index, ...other }: TabPanelProps) {
  return (
    <div
      role="tabpanel"
      hidden={value !== index}
      id={`tabpanel-${index}`}
      aria-labelledby={`tab-${index}`}
      {...other}
    >
      {value === index && <div>{children}</div>}
    </div>
  );
}
