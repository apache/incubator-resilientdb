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

import React, { useState } from 'react';
import classNames from 'classnames/bind';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faChevronDown } from '@fortawesome/free-solid-svg-icons/faChevronDown';
import styles from './Box.module.scss';

const cx = classNames.bind(styles);
/**
 * Box renders its children with a box around it
 */

export interface BoxProps {
  children: React.ReactNode;
  // Disable padding, disabled by default since it should be used for more special cases
  noPadding?: boolean;

  // Additional classnames
  className?: string;
}
export default function Box(props: BoxProps) {
  const { children, noPadding, className = '' } = props;

  const paddingClass = noPadding ? '' : styles.padding;

  return (
    <div className={`${styles.box} ${paddingClass} ${className}`}>
      {children}
    </div>
  );
}

export interface CollapseBoxProps {
  /** must be non empty */
  title: string;
  children: React.ReactNode;
}

export function CollapseBox({ title, children }: CollapseBoxProps) {
  const [collapsed, toggleCollapse] = useState(false);

  return (
    <div className={styles.collapseBox}>
      <div
        onClick={() => toggleCollapse((c) => !c)}
        className={styles.collapseTitle}
        aria-hidden
      >
        <div>{title}</div>
        <FontAwesomeIcon
          className={cx({
            [styles.collapseIcon]: true,
            [styles.collapsed]: collapsed,
          })}
          icon={faChevronDown}
        />
      </div>
      <Box
        className={cx({
          [styles.collapsedContent]: collapsed,
        })}
      >
        {children}
      </Box>
    </div>
  );
}
