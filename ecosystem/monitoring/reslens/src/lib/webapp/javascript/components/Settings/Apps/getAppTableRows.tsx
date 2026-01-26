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
import { faTimes } from '@fortawesome/free-solid-svg-icons/faTimes';

import Button from '@webapp/ui/Button';
import Icon from '@webapp/ui/Icon';
import { App } from '@webapp/models/app';
import type { BodyRow } from '@webapp/ui/Table';
import confirmDelete from '@webapp/components/Modals/ConfirmDelete';
import LoadingSpinner from '@webapp/ui/LoadingSpinner';

import styles from './AppTableItem.module.css';

interface DeleteButtorProps {
  onDelete: (app: App) => void;
  isLoading: boolean;
  app: App;
}

function DeleteButton(props: DeleteButtorProps) {
  const { onDelete, app, isLoading } = props;

  const handleDeleteClick = () => {
    confirmDelete({
      objectName: app.name,
      objectType: 'app',
      withConfirmationInput: true,
      warningMsg: `Note: This action can take up to ~15 minutes depending on the size of your application and wont' be reflected in the UI until it is complete.`,
      onConfirm: () => onDelete(app),
    });
  };

  return isLoading ? (
    <LoadingSpinner className={styles.loadingIcon} />
  ) : (
    <Button type="button" kind="danger" onClick={handleDeleteClick}>
      <Icon icon={faTimes} />
    </Button>
  );
}

export function getAppTableRows(
  displayApps: App[],
  appsInProcessing: string[],
  handleDeleteApp: (app: App) => void
): BodyRow[] {
  const bodyRows = displayApps.reduce((acc, app) => {
    const { name } = app;

    const row = {
      cells: [
        { value: name },
        {
          value: (
            <div className={styles.actions}>
              <DeleteButton
                app={app}
                onDelete={handleDeleteApp}
                isLoading={appsInProcessing.includes(name)}
              />
            </div>
          ),
          align: 'center',
        },
      ],
    };

    acc.push(row);
    return acc;
  }, [] as BodyRow[]);

  return bodyRows;
}
