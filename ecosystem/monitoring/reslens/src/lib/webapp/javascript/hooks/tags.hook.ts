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
  fetchTags,
  selectRanges,
  selectQueries,
  selectAppTags,
} from '@webapp/redux/reducers/continuous';

// useTags handle loading tags when any of the following changes
// query, from, until
// Since the backend may have new tags given a new interval
export default function useTags() {
  const dispatch = useAppDispatch();
  const ranges = useAppSelector(selectRanges);
  const queries = useAppSelector(selectQueries);
  const { leftQuery, rightQuery, query } = queries;

  const regularTags = useAppSelector(selectAppTags(query));
  const leftTags = useAppSelector(selectAppTags(leftQuery));
  const rightTags = useAppSelector(selectAppTags(rightQuery));

  useEffect(() => {
    if (leftQuery) {
      dispatch(fetchTags(leftQuery));
    }
  }, [leftQuery, JSON.stringify(ranges.left)]);

  useEffect(() => {
    if (rightQuery) {
      dispatch(fetchTags(rightQuery));
    }
  }, [rightQuery, JSON.stringify(ranges.right)]);

  useEffect(() => {
    if (query) {
      dispatch(fetchTags(query));
    }
  }, [query, JSON.stringify(ranges.regular)]);

  return {
    regularTags,
    leftTags,
    rightTags,
  };
}
