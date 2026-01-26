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
import { actions, selectQueries } from '@webapp/redux/reducers/continuous';
import { useAppDispatch, useAppSelector } from '@webapp/redux/hooks';
import { queryToAppName, Query } from '@webapp/models/query';

function isQueriesHasSameApp(queries: Query[]): boolean {
  const appName = queryToAppName(queries[0]);
  if (appName.isNothing) {
    return false;
  }

  return queries.every((query) => query.startsWith(appName.value));
}

// usePopulateLeftRightQuery populates the left and right queries using the main query
export default function usePopulateLeftRightQuery() {
  const dispatch = useAppDispatch();
  const { query, leftQuery, rightQuery } = useAppSelector(selectQueries);

  // should not populate queries when redirected
  // plus it prohibits different apps from being compared/diffed
  const shouldResetQuery =
    query && !isQueriesHasSameApp([query, leftQuery, rightQuery]);

  // When the query changes (ie the app has changed)
  // We populate left and right tags to reflect that application
  useEffect(() => {
    if (shouldResetQuery) {
      dispatch(actions.setRightQuery(query));
      dispatch(actions.setLeftQuery(query));
    }
  }, [shouldResetQuery]);
}
