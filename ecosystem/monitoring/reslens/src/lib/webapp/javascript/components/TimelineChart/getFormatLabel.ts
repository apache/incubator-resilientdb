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

import { getUTCdate, getTimelineFormatDate } from '@webapp/util/formatDate';

function getFormatLabel({
  date,
  timezone,
  xaxis,
}: {
  date: number;
  timezone?: string;
  xaxis: {
    min: number;
    max: number;
  };
}) {
  if (!xaxis) {
    return '';
  }

  try {
    const d = getUTCdate(
      new Date(date),
      timezone === 'utc' ? 0 : new Date().getTimezoneOffset()
    );

    const hours = Math.abs(xaxis.max - xaxis.min) / 60 / 60 / 1000;

    return getTimelineFormatDate(d, hours);
  } catch (e) {
    return '???';
  }
}

export default getFormatLabel;
