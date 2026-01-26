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

import { renderHook, act } from '@testing-library/react-hooks';

import { useTableSort } from './Table';

const mockHeadRow = [
  { name: 'self', label: 'test col2', sortable: 1 },
  { name: 'name', label: 'test col1', sortable: 1 },
  { name: 'total', label: 'test col3', sortable: 1 },
  { name: 'selfLeft', label: 'test col4', sortable: 1 },
  { name: 'selfRight', label: 'test col5', sortable: 1 },
  { name: 'selfDiff', label: 'test col6', sortable: 1 },
  { name: 'totalLeft', label: 'test col7', sortable: 1 },
  { name: 'totalRight', label: 'test col8', sortable: 1 },
  { name: 'totalDiff', label: 'test col9', sortable: 1 },
];

describe('Hook: useTableSort', () => {
  const render = () => renderHook(() => useTableSort(mockHeadRow)).result;

  it('should return initial sort values', () => {
    const hook = render();
    expect(hook.current).toStrictEqual({
      sortBy: 'self',
      sortByDirection: 'desc',
      updateSortParams: expect.any(Function),
    });
  });

  it('should update sort direction', () => {
    const hook = render();

    expect(hook.current.sortByDirection).toBe('desc');
    act(() => {
      hook.current.updateSortParams('self');
    });
    expect(hook.current.sortByDirection).toBe('asc');
  });

  it('should update sort value and sort direction', () => {
    const hook = render();

    expect(hook.current).toMatchObject({
      sortBy: 'self',
      sortByDirection: 'desc',
    });

    act(() => {
      hook.current.updateSortParams('name');
    });
    expect(hook.current).toMatchObject({
      sortBy: 'name',
      sortByDirection: 'desc',
    });

    act(() => {
      hook.current.updateSortParams('name');
    });
    expect(hook.current).toMatchObject({
      sortBy: 'name',
      sortByDirection: 'asc',
    });
  });
});
