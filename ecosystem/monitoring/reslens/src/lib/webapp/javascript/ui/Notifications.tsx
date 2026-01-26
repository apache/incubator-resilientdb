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
import ReactNotification, {
  store as libStore,
  ReactNotificationOptions,
  DismissOptions,
} from 'react-notifications-component';
import 'react-notifications-component/dist/scss/notification.scss';
import styles from './Notifications.module.scss';

export default function Notifications() {
  return (
    <div className={styles.notificationsComponent}>
      <ReactNotification />
    </div>
  );
}

const defaultParams: Partial<ReactNotificationOptions> = {
  insert: 'top',
  container: 'top-right',
  animationIn: ['animate__animated', 'animate__fadeIn'],
  animationOut: ['animate__animated', 'animate__fadeOut'],
};

export type NotificationOptions = {
  title?: string;
  message: string | JSX.Element;
  additionalInfo?: string[];
  type: 'success' | 'danger' | 'info' | 'warning';

  dismiss?: DismissOptions;
  onRemoval?: ((id: string, removedBy: ShamefulAny) => void) | undefined;
};

function Message({
  message,
  additionalInfo,
}: {
  message: string | JSX.Element;
  additionalInfo?: string[];
}) {
  const msg = typeof message === 'string' ? <p>{message}</p> : message;
  return (
    <div>
      {msg}
      {additionalInfo && <h4>Additional Info:</h4>}

      {additionalInfo && (
        <ul>
          {additionalInfo.map((a) => {
            return <li key={a}>{a}</li>;
          })}
        </ul>
      )}
    </div>
  );
}

export const store = {
  addNotification({
    title,
    message,
    type,
    dismiss,
    onRemoval,
    additionalInfo,
  }: NotificationOptions) {
    dismiss = dismiss || {
      duration: 5000,
      pauseOnHover: true,
      click: false,
      touch: false,
      showIcon: true,
      onScreen: true,
    };

    libStore.addNotification({
      ...defaultParams,
      title,
      message: <Message message={message} additionalInfo={additionalInfo} />,
      type,
      dismiss,
      onRemoval,
      container: 'top-right',
    });
  },
};
