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

import React, { useEffect, useState } from 'react';
import 'react-dom';
import { useAppDispatch, useAppSelector } from '@webapp/redux/hooks';
import Box from '@webapp/ui/Box';
import { FlamegraphRenderer } from '@pyroscope/flamegraph/src/FlamegraphRenderer';
import FileList from '@webapp/components/FileList';
import useExportToFlamegraphDotCom from '@webapp/components/exportToFlamegraphDotCom.hook';
import ExportData from '@webapp/components/ExportData';
import {
  uploadFile,
  fetchProfile,
  selectShared,
  fetchAllProfiles,
  selectedSelectedProfileId,
  selectProfile,
} from '@webapp/redux/reducers/adhoc';
import useColorMode from '@webapp/hooks/colorMode.hook';
import { Tabs, Tab, TabPanel } from '@webapp/ui/Tabs';
import FileUploader from './components/FileUploader';
import adhocStyles from './Adhoc.module.scss';

function AdhocSingle() {
  const dispatch = useAppDispatch();
  const { profilesList } = useAppSelector(selectShared);
  const selectedProfileId = useAppSelector(selectedSelectedProfileId('left'));
  const profile = useAppSelector(selectProfile('left'));
  useColorMode();
  const [currentTab, setCurrentTab] = useState(0);

  useEffect(() => {
    dispatch(fetchAllProfiles());
  }, [dispatch]);

  const exportToFlamegraphDotComFn = useExportToFlamegraphDotCom(
    profile.unwrapOr(undefined)
  );

  const flame = (() => {
    if (profile.isNothing) {
      return <></>;
    }

    return (
      <FlamegraphRenderer
        profile={profile.value}
        showCredit={false}
        ExportData={
          <ExportData
            flamebearer={profile.value}
            exportJSON
            exportFlamegraphDotCom
            exportFlamegraphDotComFn={exportToFlamegraphDotComFn}
          />
        }
      />
    );
  })();

  const setFile = async ({
    file,
    spyName,
    units,
  }: {
    file: File;
    spyName?: string;
    units?: string;
  }) => {
    await dispatch(uploadFile({ file, spyName, units, side: 'left' }));
    setCurrentTab(1);
  };

  return (
    <div className="main-wrapper">
      <Box>
        <Tabs value={currentTab} onChange={(e, value) => setCurrentTab(value)}>
          <Tab label="Upload" />
          <Tab label="Pyroscope data" />
        </Tabs>
        <TabPanel value={currentTab} index={0}>
          <FileUploader className={adhocStyles.tabPanel} setFile={setFile} />
        </TabPanel>
        <TabPanel value={currentTab} index={1}>
          {profilesList.type === 'loaded' && (
            <FileList
              className={adhocStyles.tabPanel}
              selectedProfileId={selectedProfileId}
              profilesList={profilesList.profilesList}
              onProfileSelected={(id: string) => {
                dispatch(fetchProfile({ id, side: 'left' }));
              }}
            />
          )}
        </TabPanel>
        {flame}
      </Box>
    </div>
  );
}

export default AdhocSingle;
