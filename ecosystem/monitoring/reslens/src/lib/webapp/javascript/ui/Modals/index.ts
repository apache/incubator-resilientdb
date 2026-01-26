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

import Swal, { SweetAlertInput, SweetAlertOptions } from 'sweetalert2';

import styles from './Modal.module.css';

const defaultParams: Partial<SweetAlertOptions> = {
  showCancelButton: true,
  allowOutsideClick: true,
  backdrop: true,
  focusConfirm: false,
  customClass: {
    popup: styles.popup,
    title: styles.title,
    input: styles.input,
    confirmButton: styles.button,
    denyButton: styles.button,
    cancelButton: styles.button,
    validationMessage: styles.validationMessage,
  },
  inputAttributes: {
    required: 'true',
  },
};

export type ShowModalParams = {
  title: string;
  html?: string;
  confirmButtonText: string;
  type: 'danger' | 'normal';
  onConfirm?: ShamefulAny;
  input?: SweetAlertInput;
  inputValue?: string;
  inputLabel?: string;
  inputPlaceholder?: string;
  validationMessage?: string;
  inputValidator?: (value: string) => string | null;
};

const ShowModal = async ({
  title,
  html,
  confirmButtonText,
  type,
  onConfirm,
  input,
  inputValue,
  inputLabel,
  inputPlaceholder,
  validationMessage,
  inputValidator,
}: ShowModalParams) => {
  const { isConfirmed, value } = await Swal.fire({
    title,
    html,
    confirmButtonText,
    input,
    inputLabel,
    inputPlaceholder,
    inputValue,
    validationMessage,
    inputValidator,
    confirmButtonColor: getButtonStyleFromType(type),
    ...defaultParams,
  });

  if (isConfirmed) {
    onConfirm(value);
  }

  return value;
};

function getButtonStyleFromType(type: 'danger' | 'normal') {
  if (type === 'danger') {
    return '#dc3545'; // red
  }

  return '#0074d9'; // blue
}

export default ShowModal;
