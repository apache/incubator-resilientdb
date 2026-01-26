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

import Color from 'color';

export const HEATMAP_HEIGHT = 250;

export const SELECTED_AREA_BACKGROUND = Color.rgb(240, 240, 240).alpha(0.5);

export const DEFAULT_HEATMAP_PARAMS = {
  minValue: 0,
  maxValue: 1000000000,
  heatmapTimeBuckets: 128,
  heatmapValueBuckets: 32,
};

// viridis palette
export const HEATMAP_COLORS = [
  Color.rgb(253, 231, 37),
  Color.rgb(174, 216, 68),
  Color.rgb(94, 201, 98),
  Color.rgb(33, 145, 140),
  Color.rgb(59, 82, 139),
  Color.rgb(64, 42, 112),
  Color.rgb(68, 1, 84),
];
