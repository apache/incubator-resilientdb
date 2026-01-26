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

import React, { useState } from 'react';
import Button from '@webapp/ui/Button';

import { useAppDispatch } from '@webapp/redux/hooks';

import { changeMyPassword } from '@webapp/redux/reducers/user';
import { addNotification } from '@webapp/redux/reducers/notifications';
import StatusMessage from '@webapp/ui/StatusMessage';
import InputField from '@webapp/ui/InputField';

function ChangePasswordForm(props: ShamefulAny) {
  const { user } = props;
  const [form, setForm] = useState<ShamefulAny>({ errors: [] });
  const dispatch = useAppDispatch();
  if (user.isExternal) {
    return null;
  }

  const handleChange = (e: ShamefulAny) => {
    setForm({ ...form, [e.target.name]: e.target.value });
  };

  const handleFormSubmit = (evt: ShamefulAny) => {
    evt.preventDefault();
    if (form.password !== form.passwordAgain) {
      return setForm({ errors: ['Passwords must match'] });
    }
    dispatch(
      changeMyPassword({
        oldPassword: form.oldPassword,
        newPassword: form.password,
      })
    )
      .unwrap()
      .then(
        () => {
          dispatch(
            addNotification({
              type: 'success',
              title: 'Password changed',
              message: `Password has been successfully changed`,
            })
          );
          setForm({
            errors: [],
            oldPassword: '',
            password: '',
            passwordAgain: '',
          });
        },
        (e) => setForm({ errors: e.errors })
      );
    return false;
  };

  return (
    <>
      <h2>Change password</h2>
      <div>
        <form onSubmit={handleFormSubmit}>
          <StatusMessage type="error" message={form.errors.join(', ')} />
          <InputField
            label="Old password"
            type="password"
            placeholder="Password"
            name="oldPassword"
            required
            onChange={handleChange}
            value={form.oldPassword}
          />
          <InputField
            label="New password"
            type="password"
            placeholder="New password"
            name="password"
            required
            onChange={handleChange}
            value={form.password}
          />
          <InputField
            label="Confirm new password"
            type="password"
            placeholder="New password"
            name="passwordAgain"
            required
            onChange={handleChange}
            value={form.passwordAgain}
          />
          <Button type="submit" kind="secondary">
            Save
          </Button>
        </form>
      </div>
    </>
  );
}

export default ChangePasswordForm;
