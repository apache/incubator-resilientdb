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

/* eslint-disable no-unused-expressions */
import React, { useEffect, useMemo, ChangeEvent, useRef } from 'react';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faLink } from '@fortawesome/free-solid-svg-icons/faLink';
import Input from '../webapp/javascript/ui/Input';
import { Tooltip } from '../webapp/javascript/ui/Tooltip';
import styles from './SharedQueryInput.module.scss';
import type { ProfileHeaderProps } from './Toolbar';

interface SharedQueryProps {
  onHighlightChange: ProfileHeaderProps['handleSearchChange'];
  highlightQuery: ProfileHeaderProps['highlightQuery'];
  sharedQuery: ProfileHeaderProps['sharedQuery'];
  width: number;
}

const usePreviousSyncEnabled = (syncEnabled?: string | boolean) => {
  const ref = useRef();

  useEffect(() => {
    (ref.current as string | boolean | undefined) = syncEnabled;
  });

  return ref.current;
};

const SharedQueryInput = ({
  onHighlightChange,
  highlightQuery,
  sharedQuery,
  width,
}: SharedQueryProps) => {
  const prevSyncEnabled = usePreviousSyncEnabled(sharedQuery?.syncEnabled);

  const onQueryChange = (e: ChangeEvent<HTMLInputElement>) => {
    onHighlightChange(e.target.value);

    if (sharedQuery && sharedQuery.syncEnabled) {
      sharedQuery.onQueryChange(e.target.value);
    }
  };

  useEffect(() => {
    if (typeof sharedQuery?.searchQuery === 'string') {
      if (sharedQuery.syncEnabled) {
        onHighlightChange(sharedQuery.searchQuery);
      }

      if (
        !sharedQuery.syncEnabled &&
        prevSyncEnabled &&
        prevSyncEnabled !== sharedQuery?.id
      ) {
        onHighlightChange('');
      }
    }
  }, [sharedQuery?.searchQuery, sharedQuery?.syncEnabled]);

  const onToggleSync = () => {
    const newValue = sharedQuery?.syncEnabled ? false : sharedQuery?.id;
    sharedQuery?.toggleSync(newValue as string | false);

    if (newValue) {
      sharedQuery?.onQueryChange(highlightQuery);
    } else {
      onHighlightChange(highlightQuery);
      sharedQuery?.onQueryChange('');
    }
  };

  const inputValue = useMemo(
    () =>
      sharedQuery && sharedQuery.syncEnabled
        ? sharedQuery.searchQuery || ''
        : highlightQuery,
    [sharedQuery, highlightQuery]
  );

  const inputClassName = useMemo(
    () =>
      `${sharedQuery ? styles.searchWithSync : styles.search} ${
        sharedQuery?.syncEnabled ? styles['search-synced'] : ''
      }`,
    [sharedQuery]
  );

  return (
    <div className={styles.wrapper} style={{ width }}>
      <Input
        testId="flamegraph-search"
        className={inputClassName}
        type="search"
        name="flamegraph-search"
        placeholder="Search…"
        minLength={2}
        debounceTimeout={100}
        onChange={onQueryChange}
        value={inputValue}
      />
      {sharedQuery ? (
        <Tooltip
          placement="top"
          title={
            sharedQuery.syncEnabled ? 'Unsync search bars' : 'Sync search bars'
          }
        >
          <button
            className={
              sharedQuery.syncEnabled ? styles.syncSelected : styles.sync
            }
            onClick={onToggleSync}
          >
            <FontAwesomeIcon
              className={`${
                sharedQuery.syncEnabled ? styles.checked : styles.icon
              }`}
              icon={faLink}
            />
          </button>
        </Tooltip>
      ) : null}
    </div>
  );
};

export default SharedQueryInput;
