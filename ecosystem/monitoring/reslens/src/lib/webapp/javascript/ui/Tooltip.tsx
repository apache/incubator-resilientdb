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

/* eslint-disable 
jsx-a11y/click-events-have-key-events, 
jsx-a11y/no-noninteractive-element-interactions, 
css-modules/no-unused-class 
*/
import React from 'react';
import MuiTooltip from '@mui/material/Tooltip';
import styles from './Tooltip.module.scss';

// Don't expose all props from the lib
type AvailableProps = Pick<
  React.ComponentProps<typeof MuiTooltip>,
  'title' | 'children' | 'placement'
>;
function Tooltip(props: AvailableProps) {
  const defaultProps: Omit<
    React.ComponentProps<typeof MuiTooltip>,
    'title' | 'children'
  > = {
    arrow: true,
    classes: {
      tooltip: styles.muiTooltip,
      arrow: styles.muiTooltipArrow,
    },
  };

  /* eslint-disable-next-line react/jsx-props-no-spreading */
  return <MuiTooltip {...defaultProps} {...props} />;
}

export { Tooltip };
