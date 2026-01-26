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

/* eslint-disable */
/**
 * @remarks
 * function taken from built-in markings support in Flot
 * @param plot - plot instance
 * @param coord - 'x' | 'y'
 *
 * @returns params of X or Y axis
 *
 */
export default function extractRange(plot: jquery.flot.plot, coord: 'x' | 'y') {
  const axes = plot.getAxes();
  var axis, from, to, key;
  var ranges: { [x: string]: any } = axes;

  for (var k in axes) {
    // @ts-ignore:next-line
    axis = axes[k];
    if (axis.direction == coord) {
      key = coord + axis.n + 'axis';
      if (!ranges[key] && axis.n == 1) key = coord + 'axis'; // support x1axis as xaxis
      if (ranges[key]) {
        from = ranges[key].from;
        to = ranges[key].to;
        break;
      }
    }
  }

  // backwards-compat stuff - to be removed in future
  if (!ranges[key as string]) {
    axis = coord == 'x' ? plot.getXAxes()[0] : plot.getYAxes()[0];
    from = ranges[coord + '1'];
    to = ranges[coord + '2'];
  }

  // auto-reverse as an added bonus
  if (from != null && to != null && from > to) {
    var tmp = from;
    from = to;
    to = tmp;
  }

  return { from: from, to: to, axis: axis };
}
