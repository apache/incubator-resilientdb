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

import { render, screen } from '@testing-library/react';
import React, { useState } from 'react';
import { Tab, TabPanel, Tabs } from './Tabs';
import userEvent from '@testing-library/user-event';

function TabsComponent() {
  const [value, setTab] = useState(0);

  return (
    <>
      <Tabs value={value} onChange={(e, value) => setTab(value)}>
        <Tab label="Tab_1" />
        <Tab label="Tab_2" />
      </Tabs>
      <TabPanel value={value} index={0}>
        Tab_1_Content
      </TabPanel>
      <TabPanel value={value} index={1}>
        Tab_2_Content
      </TabPanel>
    </>
  );
}

describe('Tabs', () => {
  beforeEach(() => {
    render(<TabsComponent />);
  });

  it('shows all tabs', () => {
    expect(screen.getByText('Tab_1')).toBeVisible();
    expect(screen.getByText('Tab_2')).toBeVisible();
  });

  it('toggling tabs works', () => {
    expect(screen.getByText('Tab_1_Content')).toBeVisible();
    expect(screen.queryByText('Tab_2_Content')).toBeNull();

    userEvent.click(screen.getByText('Tab_2'), { button: 1 });

    expect(screen.getByText('Tab_2_Content')).toBeVisible();
    expect(screen.queryByText('Tab_1_Content')).toBeNull();
  });
});
