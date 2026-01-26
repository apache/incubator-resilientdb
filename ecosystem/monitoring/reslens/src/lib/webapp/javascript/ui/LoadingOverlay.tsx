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

import React, { ReactNode, useEffect, useState } from 'react';
import LoadingSpinner from '@webapp/ui/LoadingSpinner';
import cx from 'classnames';
import styles from './LoadingOverlay.module.css';

/**
 * LoadingOverlay when 'active' will cover the entire parent's width/height with
 * an overlay and a loading spinner
 */
export function LoadingOverlay({
  active = true,
  spinnerPosition = 'center',
  children,
  delay = 250,
}: {
  /** where to position the spinner. use baseline when the component's vertical center is outside the viewport */
  spinnerPosition?: 'center' | 'baseline';
  active: boolean;
  children?: ReactNode;
  /** delay in ms before showing the overlay. this evicts a flick */
  delay?: number;
}) {
  const [isVisible, setVisible] = useState(false);

  // Wait for `delay` ms before showing the overlay
  // So that it feels snappy when server is fast
  // https://www.nngroup.com/articles/progress-indicators/
  useEffect(() => {
    if (active) {
      const timeoutID = window.setTimeout(() => {
        setVisible(true);
      }, delay);

      return () => clearTimeout(timeoutID);
    }

    setVisible(false);
    return () => {};
  }, [active]);

  return (
    <div>
      <div
        className={cx(
          styles.loadingOverlay,
          !isVisible ? styles.unactive : null
        )}
        style={{
          alignItems: spinnerPosition,
        }}
      >
        <LoadingSpinner size="46px" />
      </div>
      {children}
    </div>
  );
}
