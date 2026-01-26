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

import ShowModal, { ShowModalParams } from '@webapp/ui/Modals';

interface ConfirmDeleteProps {
  objectType: string;
  objectName: string;
  onConfirm: () => void;
  warningMsg?: string;
  withConfirmationInput?: boolean;
}

function confirmDelete({
  objectName,
  objectType,
  onConfirm,
  withConfirmationInput,
  warningMsg,
}: ConfirmDeleteProps) {
  const confirmationInputProps: Partial<ShowModalParams> = withConfirmationInput
    ? {
        input: 'text' as ShowModalParams['input'],
        inputLabel: `To confirm deletion enter ${objectType} name below.`,
        inputPlaceholder: objectName,
        inputValidator: (value) =>
          value === objectName ? null : 'Name does not match',
      }
    : {};

  // eslint-disable-next-line @typescript-eslint/no-floating-promises
  ShowModal({
    title: `Delete ${objectType}`,
    html: `Are you sure you want to delete<br><strong>${objectName}</strong> ?${
      warningMsg ? `<br><br>${warningMsg}` : ''
    }`,
    confirmButtonText: 'Delete',
    type: 'danger',
    onConfirm,
    ...confirmationInputProps,
  });
}

export default confirmDelete;
