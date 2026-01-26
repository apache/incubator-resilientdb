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

import React, { useEffect } from 'react';
import { useAppSelector, useAppDispatch } from '@webapp/redux/hooks';
import {
  reloadAppNames,
  selectAppNames,
  setQuery,
  selectApplicationName,
} from '@webapp/redux/reducers/continuous';
import { queryFromAppName } from '@webapp/models/query';

export default function Continuous({
  children,
}: {
  children: React.ReactElement;
}) {
  const dispatch = useAppDispatch();
  const appNames = useAppSelector(selectAppNames);
  const selectedAppName = useAppSelector(selectApplicationName);

  useEffect(() => {
    async function run() {
      await dispatch(reloadAppNames());
    }

    run();
  }, [dispatch]);

  // Pick the first one if there's nothing selected
  useEffect(() => {
    if (!selectedAppName && appNames.length > 0) {
      dispatch(setQuery(queryFromAppName(appNames[0])));
    }
  }, [appNames, selectedAppName]);

  return children;
}
