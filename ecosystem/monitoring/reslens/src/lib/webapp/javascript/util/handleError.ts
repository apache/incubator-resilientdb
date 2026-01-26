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

import type { RequestError, RequestNotOkError } from '@webapp/services/base';
import { ZodError } from 'zod';
import { addNotification } from '@webapp/redux/reducers/notifications';
import { useAppDispatch } from '@webapp/redux/hooks';

/**
 * handleError handles service errors
 */
export default async function handleError(
  dispatch: ReturnType<typeof useAppDispatch>,
  message: string,
  error: ZodError | RequestError
): Promise<void> {
  // We log the error in case a tech-savy user wants to debug themselves
  console.error(error);

  let errorMessage;
  if ('message' in error) {
    errorMessage = error.message;
  }

  // a ZodError means its format is not what we expect
  if (error instanceof ZodError) {
    errorMessage = 'response not in the expected format';
  }

  // display a notification
  dispatch(
    addNotification({
      title: 'Error',
      message: [message, errorMessage].join('\n'),
      type: 'danger',
    })
  );
}
