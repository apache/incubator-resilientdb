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

import React, { ReactNode } from 'react';
import cx from 'classnames';
import styles from './StatusMessage.module.scss';

interface StatusMessageProps {
  type: 'error' | 'success' | 'warning' | 'info';
  message: string;
  action?: ReactNode;
}

export default function StatusMessage({
  type,
  message,
  action,
}: StatusMessageProps) {
  const getClassnameForType = () => {
    switch (type) {
      case 'error':
        return styles.error;
      case 'success':
        return styles.success;
      case 'warning':
        return styles.warning;
      case 'info':
        return styles.info;
      default:
        return styles.error;
    }
  };

  return (
    <div
      className={cx({
        [styles.statusMessage]: true,
        [getClassnameForType()]: true,
      })}
    >
      <div>{message}</div>
      <div className={styles.action}>{action}</div>
    </div>
  );
}
