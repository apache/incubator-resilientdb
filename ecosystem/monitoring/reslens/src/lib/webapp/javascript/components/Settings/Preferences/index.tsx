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

/* eslint-disable prettier/prettier */
import React, { useState } from 'react';
import Button from '@webapp/ui/Button';
import { useAppDispatch } from '@webapp/redux/hooks';

import { editMe, withCurrentUser } from '@webapp/redux/reducers/user';
import { addNotification } from '@webapp/redux/reducers/notifications';

import StatusMessage from '@webapp/ui/StatusMessage';
import InputField from '@webapp/ui/InputField';

function Preferences(props: ShamefulAny) {
  const { currentUser } = props;
  const dispatch = useAppDispatch();

  const [form, setForm] = useState(currentUser);
  const handleFormSubmit = (evt: ShamefulAny) => {
    evt.preventDefault();

    dispatch(editMe(form))
      .unwrap()
      .then(() => {
        dispatch(
          addNotification({
            type: 'success',
            title: 'Success',
            message: 'User has been successfully edited',
          })
        );
      });
    return false;
  };

  const handleFormChange = (event: ShamefulAny) => {
    const { name } = event.target;
    const { value } = event.target;
    setForm({ ...form, [name]: value });
  };

  const isEditDisabled = !!(currentUser && currentUser.isExternal);

  if (!currentUser) return <></>;
  return (
    <>
      <h2>Edit profile</h2>
      <form onSubmit={handleFormSubmit}>
        <StatusMessage type="error" message={form.errors} />
        <InputField
          label="Username"
          type="text"
          placeholder="username"
          value={form?.name}
          name="name"
          required
          disabled={isEditDisabled}
          onChange={handleFormChange}
        />
        <InputField
          label="Full Name"
          type="text"
          placeholder="Full Name"
          name="fullName"
          value={form?.fullName}
          onChange={handleFormChange}
        />
        <InputField
          label="Email"
          type="text"
          placeholder="email"
          value={form?.email}
          name="email"
          onChange={handleFormChange}
        />
        <Button type="submit" kind="secondary">
          Save
        </Button>
      </form>
    </>
  );
}

export default withCurrentUser(Preferences);
