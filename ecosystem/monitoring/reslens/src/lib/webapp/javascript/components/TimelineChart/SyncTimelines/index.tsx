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
import Button from '@webapp/ui/Button';
import { TimelineData } from '@webapp/components/TimelineChart/TimelineChartWrapper';
import StatusMessage from '@webapp/ui/StatusMessage';
import { useSync } from './useSync';
import styles from './styles.module.scss';

interface SyncTimelinesProps {
  timeline: TimelineData;
  leftSelection: {
    from: string;
    to: string;
  };
  rightSelection: {
    from: string;
    to: string;
  };
  onSync: (from: string, until: string) => void;
  comparisonModeActive?: boolean;
  isDataLoading?: boolean;
}

function SyncTimelines({
  timeline,
  leftSelection,
  rightSelection,
  onSync,
  comparisonModeActive = false,
  isDataLoading = false,
}: SyncTimelinesProps) {
  const { isWarningHidden, onIgnore, title, onSyncClick } = useSync({
    timeline,
    leftSelection,
    rightSelection,
    onSync,
  });

  if (isWarningHidden || comparisonModeActive || isDataLoading) {
    return null;
  }

  return (
    <StatusMessage
      type="warning"
      message={title}
      action={
        <div className={styles.buttons}>
          <Button
            kind="outline"
            onClick={onIgnore}
            className={styles.ignoreButton}
          >
            Ignore
          </Button>
          <Button
            kind="outline"
            onClick={onSyncClick}
            className={styles.syncButton}
          >
            Sync Timelines
          </Button>
        </div>
      }
    />
  );
}

export default SyncTimelines;
