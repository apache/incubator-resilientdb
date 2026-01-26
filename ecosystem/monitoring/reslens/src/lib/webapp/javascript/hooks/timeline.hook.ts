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

import { useEffect } from 'react';
import { useAppDispatch, useAppSelector } from '@webapp/redux/hooks';
import {
  fetchSideTimelines,
  selectContinuousState,
  selectTimelineSidesData,
} from '@webapp/redux/reducers/continuous';
import Color from 'color';

// Purple
export const leftColor = Color('rgb(208, 102, 212)');
// Blue
export const rightColor = Color('rgb(19, 152, 246)');
// Greyish
export const selectionColor = Color('rgb(240, 240, 240)');

export default function useTimelines() {
  const dispatch = useAppDispatch();
  const {
    from,
    until,
    refreshToken,
    maxNodes,

    leftQuery,
    rightQuery,
  } = useAppSelector(selectContinuousState);
  const timelines = useAppSelector(selectTimelineSidesData);

  // Only reload timelines when an item that affects a timeline has changed
  useEffect(() => {
    dispatch(fetchSideTimelines(null));
  }, [from, until, refreshToken, maxNodes, leftQuery, rightQuery]);

  const leftTimeline = {
    color: leftColor.rgb().toString(),
    data: timelines.left,
  };

  const rightTimeline = {
    color: rightColor.rgb().toString(),
    data: timelines.right,
  };
  return {
    leftTimeline,
    rightTimeline,
  };
}
