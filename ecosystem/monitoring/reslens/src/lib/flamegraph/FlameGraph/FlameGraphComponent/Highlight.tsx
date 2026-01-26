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

import { Maybe } from 'true-myth';
import React from 'react';
import { DeepReadonly } from 'ts-essentials';
import styles from './Highlight.module.css';

export interface HighlightProps {
  // probably the same as the bar height
  barHeight: number;
  zoom: Maybe<DeepReadonly<{ i: number; j: number }>>;
  xyToHighlightData: (
    x: number,
    y: number
  ) => Maybe<{
    left: number;
    top: number;
    width: number;
  }>;

  canvasRef: React.RefObject<HTMLCanvasElement>;
}
export default function Highlight(props: HighlightProps) {
  const { canvasRef, barHeight, xyToHighlightData, zoom } = props;
  const [style, setStyle] = React.useState<React.CSSProperties>({
    height: '0px',
    visibility: 'hidden',
  });

  React.useEffect(() => {
    // stops highlighting every time a node is zoomed or unzoomed
    // then, when a mouse move event is detected,
    // listeners are triggered and highlighting becomes visible again
    setStyle({
      height: '0px',
      visibility: 'hidden',
    });
  }, [zoom]);

  const onMouseMove = (e: MouseEvent) => {
    const opt = xyToHighlightData(e.offsetX, e.offsetY);

    if (opt.isJust) {
      const data = opt.value;

      setStyle({
        visibility: 'visible',
        height: `${barHeight}px`,
        ...data,
      });
    } else {
      // it doesn't map to a valid xy
      // so it means we are hovering out
      onMouseOut();
    }
  };

  const onMouseOut = () => {
    setStyle({
      ...style,
      visibility: 'hidden',
    });
  };

  React.useEffect(
    () => {
      // use closure to "cache" the current canvas reference
      // so that when cleaning up, it points to a valid canvas
      // (otherwise it would be null)
      const canvasEl = canvasRef.current;
      if (!canvasEl) {
        return () => {};
      }

      // watch for mouse events on the bar
      canvasEl.addEventListener('mousemove', onMouseMove);
      canvasEl.addEventListener('mouseout', onMouseOut);

      return () => {
        canvasEl.removeEventListener('mousemove', onMouseMove);
        canvasEl.removeEventListener('mouseout', onMouseOut);
      };
    },

    // refresh callback functions when they change
    [canvasRef.current, onMouseMove, onMouseOut]
  );

  return (
    <div
      className={styles.highlight}
      style={style}
      data-testid="flamegraph-highlight"
    />
  );
}
