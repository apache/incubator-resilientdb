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

import type { Heatmap } from '@webapp/services/render';

import { getTimelineFormatDate, getUTCdate } from '@webapp/util/formatDate';
import { SELECTED_AREA_BACKGROUND, HEATMAP_HEIGHT } from './constants';
import type { SelectedAreaCoordsType } from './useHeatmapSelection.hook';

export const drawRect = (
  canvas: HTMLCanvasElement,
  x: number,
  y: number,
  w: number,
  h: number
) => {
  clearRect(canvas);
  const ctx = canvas.getContext('2d') as CanvasRenderingContext2D;

  ctx.fillStyle = SELECTED_AREA_BACKGROUND.toString();
  ctx.globalAlpha = 1;
  ctx.fillRect(x, y, w, h);
};

export const clearRect = (canvas: HTMLCanvasElement) => {
  const ctx = canvas.getContext('2d') as CanvasRenderingContext2D;

  ctx.clearRect(0, 0, canvas.width, canvas.height);
};

export const sortCoordinates = (
  v1: number,
  v2: number
): { smaller: number; bigger: number } => {
  const isFirstBigger = v1 > v2;

  return {
    smaller: isFirstBigger ? v2 : v1,
    bigger: isFirstBigger ? v1 : v2,
  };
};

interface SelectionData {
  selectionMinValue: number;
  selectionMaxValue: number;
  selectionStartTime: number;
  selectionEndTime: number;
}

export const getTimeDataByXCoord = (
  heatmap: Heatmap,
  heatmapW: number,
  x: number
) => {
  const unitsForPixel = (heatmap.endTime - heatmap.startTime) / heatmapW;

  return x * unitsForPixel + heatmap.startTime;
};

export const getBucketsDurationByYCoord = (heatmap: Heatmap, y: number) => {
  const unitsForPixel = (heatmap.maxValue - heatmap.minValue) / HEATMAP_HEIGHT;

  return (HEATMAP_HEIGHT - y) * unitsForPixel;
};

export const getSelectionData = (
  heatmap: Heatmap,
  heatmapW: number,
  startCoords: SelectedAreaCoordsType,
  endCoords: SelectedAreaCoordsType,
  isClickOnYBottomEdge?: boolean
): SelectionData => {
  const timeForPixel = (heatmap.endTime - heatmap.startTime) / heatmapW;
  const valueForPixel = (heatmap.maxValue - heatmap.minValue) / HEATMAP_HEIGHT;

  const { smaller: smallerX, bigger: biggerX } = sortCoordinates(
    startCoords.x,
    endCoords.x
  );
  const { smaller: smallerY, bigger: biggerY } = sortCoordinates(
    HEATMAP_HEIGHT - startCoords.y,
    HEATMAP_HEIGHT - endCoords.y
  );

  // to fetch correct profiles when clicking on edge cells
  const selectionMinValue = Math.round(
    valueForPixel * smallerY + heatmap.minValue
  );

  return {
    selectionMinValue: isClickOnYBottomEdge
      ? selectionMinValue - 1
      : selectionMinValue,
    selectionMaxValue: Math.round(valueForPixel * biggerY + heatmap.minValue),
    selectionStartTime: timeForPixel * smallerX + heatmap.startTime,
    selectionEndTime: timeForPixel * biggerX + heatmap.startTime,
  };
};

// TODO(dogfrogfog): refactor (reuse existing formatters)
export const timeFormatter =
  (min: number, max: number, timezone: string) => (v: number) => {
    const d = getUTCdate(
      new Date(v / 1000000),
      timezone === 'utc' ? 0 : new Date().getTimezoneOffset()
    );
    // nanoseconds -> hours
    const hours = (max - min) / 60 / 60 / 1000 / 1000 / 1000;

    return getTimelineFormatDate(d, hours);
  };

// TODO(dogfrogfog): refactor types
interface TickOptions {
  formatter?: ShamefulAny;
  ticksCount: number;
  timezone?: string;
}

export const getTicks = (
  min: number,
  max: number,
  options: TickOptions,
  sampleRate?: number
): string[] => {
  let formatter;
  if (sampleRate && options.formatter) {
    formatter = (v: number) => options.formatter.format(v, sampleRate, false);
  } else {
    formatter = timeFormatter(min, max, options.timezone as string);
  }

  const step = (max - min) / options.ticksCount;
  const ticksArray = [formatter(min)];

  for (let i = 1; i <= options.ticksCount; i += 1) {
    ticksArray.push(formatter(min + step * i));
  }

  return ticksArray;
};
