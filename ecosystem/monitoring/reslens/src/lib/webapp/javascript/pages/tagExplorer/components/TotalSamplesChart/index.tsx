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
import { TimelineGroupData } from '@webapp/components/TimelineChart/TimelineChartWrapper';
import { getFormatter } from '@pyroscope/flamegraph/src/format/format';
import { Profile } from '@pyroscope/models/src';
import LoadingSpinner from '@webapp/ui/LoadingSpinner';
import PieChart, { PieChartDataItem } from './PieChart';
import PieChartTooltip from './PieChartTooltip';
import { calculateTotal } from '../../../math';
import { formatValue } from '../../../formatTableData';
import styles from './index.module.scss';

interface TotalSamplesChartProps {
  filteredGroupsData: TimelineGroupData[];
  profile?: Profile;
  formatter?: ReturnType<typeof getFormatter>;
  isLoading: boolean;
}

const CHART_HEIGT = '280px';
const CHART_WIDTH = '280px';

const TotalSamplesChart = ({
  filteredGroupsData,
  formatter,
  profile,
  isLoading,
}: TotalSamplesChartProps) => {
  const pieChartData: PieChartDataItem[] = useMemo(() => {
    return filteredGroupsData.length
      ? filteredGroupsData.map((d) => ({
          label: d.tagName,
          data: calculateTotal(d.data.samples),
          color: d.color,
        }))
      : [];
  }, [filteredGroupsData]);

  if (!pieChartData.length || isLoading) {
    return (
      <div
        style={{ width: CHART_WIDTH, height: CHART_HEIGT }}
        className={styles.chartSkeleton}
      >
        <LoadingSpinner />
      </div>
    );
  }

  return (
    <PieChart
      data={pieChartData}
      id="total-samples-chart"
      height={CHART_HEIGT}
      width={CHART_WIDTH}
      onHoverTooltip={(data) => (
        <PieChartTooltip
          label={data.label}
          value={formatValue({ formatter, profile, value: data.value })}
          percent={data.percent}
        />
      )}
    />
  );
};

export default TotalSamplesChart;
