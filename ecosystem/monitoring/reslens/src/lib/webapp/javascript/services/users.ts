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

import { Result } from '@webapp/util/fp';
import {
  Users,
  type User,
  userModel,
  usersModel,
  passwordEncode,
} from '@webapp/models/users';
import type { ZodError } from 'zod';
import { modelToResult } from '@webapp/models/utils';
import { request, parseResponse } from './base';
import type { RequestError } from './base';

export interface FetchUsersError {
  message?: string;
}

export async function fetchUsers(): Promise<
  Result<Users, RequestError | ZodError>
> {
  const response = await request('/api/users');

  if (response.isOk) {
    return parseResponse(response, usersModel);
  }

  return Result.err<Users, RequestError>(response.error);
}

export async function disableUser(
  user: User
): Promise<Result<boolean, RequestError | ZodError>> {
  const response = await request(`/api/users/${user.id}/disable`, {
    method: 'PUT',
  });

  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<false, RequestError>(response.error);
}

export async function enableUser(
  user: User
): Promise<Result<boolean, RequestError | ZodError>> {
  const response = await request(`/api/users/${user.id}/enable`, {
    method: 'PUT',
  });

  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<false, RequestError>(response.error);
}

export async function createUser(
  user: User
): Promise<Result<boolean, RequestError | ZodError>> {
  const response = await request(`/api/users`, {
    method: 'POST',
    body: JSON.stringify(user),
  });

  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<false, RequestError>(response.error);
}

export async function loadCurrentUser(): Promise<
  Result<User, RequestError | ZodError>
> {
  const response = await request(`/api/user`);
  if (response.isOk) {
    return modelToResult<User>(userModel, response.value);
  }

  return Result.err<User, RequestError>(response.error);
}

export async function logIn({
  username,
  password,
}: {
  username: string;
  password: string;
}): Promise<Result<unknown, RequestError | ZodError>> {
  const response = await request(`/login`, {
    method: 'POST',
    body: JSON.stringify({ username, password: passwordEncode(password) }),
  });
  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<boolean, RequestError>(response.error);
}

export async function signUp(data: {
  username: string;
  password: string;
  fullName: string;
  email: string;
}): Promise<Result<unknown, RequestError | ZodError>> {
  const response = await request(`/signup`, {
    method: 'POST',
    body: JSON.stringify({
      ...data,
      name: data.username,
      password: passwordEncode(data.password),
    }),
  });
  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<boolean, RequestError>(response.error);
}

export async function changeMyPassword(
  oldPassword: string,
  newPassword: string
): Promise<Result<boolean, RequestError | ZodError>> {
  const response = await request(`/api/user/password`, {
    method: 'PUT',
    body: JSON.stringify({
      oldPassword: passwordEncode(oldPassword),
      newPassword: passwordEncode(newPassword),
    }),
  });
  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<boolean, RequestError>(response.error);
}

export async function changeUserRole(
  userId: number,
  role: string
): Promise<Result<boolean, RequestError | ZodError>> {
  const response = await request(`/api/users/${userId}/role`, {
    method: 'PUT',
    body: JSON.stringify({ role }),
  });

  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<boolean, RequestError>(response.error);
}

export async function editMyUser(
  data: Partial<User>
): Promise<Result<boolean, RequestError | ZodError>> {
  const response = await request(`/api/users`, {
    method: 'PATCH',
    body: JSON.stringify(data),
  });

  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<boolean, RequestError>(response.error);
}

export async function deleteUser(data: {
  id: number;
}): Promise<Result<boolean, RequestError | ZodError>> {
  const { id } = data;
  const response = await request(`/api/users/${id}`, {
    method: 'DELETE',
  });

  if (response.isOk) {
    return Result.ok(true);
  }

  return Result.err<boolean, RequestError>(response.error);
}
