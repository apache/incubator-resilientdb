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
import ReactFlot from 'react-flot';
import Color from 'color';
import TooltipWrapper, {
  ITooltipWrapperProps,
} from '@webapp/components/TimelineChart/TooltipWrapper';
import styles from './styles.module.scss';
import 'react-flot/flot/jquery.flot.pie';
import './Interactivity.plugin';

export type PieChartDataItem = {
  label: string;
  data: number;
  color: Color | string | undefined;
};

interface TooltipProps {
  label?: string;
  percent?: number;
  value?: number;
}

interface PieChartProps {
  data: PieChartDataItem[];
  width: string;
  height: string;
  id: string;
  onHoverTooltip?: React.FC<TooltipProps>;
}

const setOnHoverDisplayTooltip = (
  data: TooltipProps & ITooltipWrapperProps,
  onHoverTooltip: React.FC<TooltipProps>
) => {
  const TooltipBody = onHoverTooltip;

  if (TooltipBody) {
    return (
      <TooltipWrapper align={data.align} pageY={data.pageY} pageX={data.pageX}>
        <TooltipBody
          value={data.value}
          label={data.label}
          percent={data.percent}
        />
      </TooltipWrapper>
    );
  }

  return null;
};

const PieChart = ({
  data,
  width,
  height,
  id,
  onHoverTooltip,
}: PieChartProps) => {
  const options = {
    series: {
      pie: {
        show: true,
        radius: 1,
        stroke: {
          width: 0,
        },
        label: {
          show: true,
          radius: 0.7,
          threshold: 0.05,
          formatter: (_: string, data: { percent: number }) =>
            `${data.percent.toFixed(2)}%`,
        },
      },
    },
    legend: {
      show: false,
    },
    grid: {
      hoverable: true,
      clickable: false,
    },
    pieChartTooltip: onHoverTooltip
      ? (tooltipData: TooltipProps & ITooltipWrapperProps) =>
          setOnHoverDisplayTooltip(tooltipData, onHoverTooltip)
      : null,
  };

  return (
    <div className={styles.wrapper}>
      <ReactFlot
        id={id}
        options={options}
        data={data}
        width={width}
        height={height}
      />
    </div>
  );
};

export default PieChart;
