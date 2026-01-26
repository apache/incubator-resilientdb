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
import { Maybe } from 'true-myth';
import styles from './ContextMenuHighlight.module.css';

export interface HighlightProps {
  // probably the same as the bar height
  barHeight: number;

  node: Maybe<{ top: number; left: number; width: number }>;
}

const initialSyle: React.CSSProperties = {
  height: '0px',
  visibility: 'hidden',
};

/**
 * Highlight on the node that triggered the context menu
 */
export default function ContextMenuHighlight(props: HighlightProps) {
  const { node, barHeight } = props;
  const [style, setStyle] = React.useState(initialSyle);

  React.useEffect(
    () => {
      node.match({
        Nothing: () => setStyle(initialSyle),
        Just: (data) =>
          setStyle({
            visibility: 'visible',
            height: `${barHeight}px`,
            ...data,
          }),
      });
    },
    // refresh callback functions when they change
    [node]
  );

  return (
    <div
      className={styles.highlightContextMenu}
      style={style}
      data-testid="flamegraph-highlight-contextmenu"
    />
  );
}
