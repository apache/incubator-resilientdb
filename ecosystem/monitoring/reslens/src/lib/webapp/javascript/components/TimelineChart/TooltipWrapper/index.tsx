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

import React, { useMemo } from 'react';
import classNames from 'classnames/bind';
import styles from './styles.module.scss';

const cx = classNames.bind(styles);

const EXPLORE_TOOLTIP_WRAPPER_ID = 'explore_tooltip_wrapper';

export interface ITooltipWrapperProps {
  pageX: number;
  pageY: number;
  align: 'left' | 'right';
  children: React.ReactNode | React.ReactNode[];
  className?: string;
}

const TooltipWrapper = ({
  className,
  pageX,
  pageY,
  align,
  children,
}: ITooltipWrapperProps) => {
  const isHidden = useMemo(() => pageX < 0 || pageY < 0, [pageX, pageY]);

  const style =
    align === 'right'
      ? { top: pageY, left: pageX + 20, right: 'auto' }
      : { top: pageY, left: 'auto', right: window.innerWidth - (pageX - 20) };

  return (
    <div
      style={style}
      className={cx({
        [styles.tooltip]: true,
        [styles.hidden]: isHidden,
        [className || '']: className,
      })}
      id={EXPLORE_TOOLTIP_WRAPPER_ID}
    >
      {children}
    </div>
  );
};

export default TooltipWrapper;
