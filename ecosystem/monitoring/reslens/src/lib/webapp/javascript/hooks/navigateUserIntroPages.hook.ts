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

import { useEffect } from 'react';
import { useAppDispatch, useAppSelector } from '@webapp/redux/hooks';
import {
  loadCurrentUser,
  selectCurrentUser,
} from '@webapp/redux/reducers/user';
import { useHistory } from 'react-router-dom';
import { PAGES } from '@webapp/pages/constants';

export default function useNavigateUserIntroPages() {
  const dispatch = useAppDispatch();
  const currentUser = useAppSelector(selectCurrentUser);
  const history = useHistory();

  // loading user on page mount
  useEffect(() => {
    dispatch(loadCurrentUser());
  }, []);
  // there are cases when user doesn't exist on page mount
  // but appears after submitting login/signup form
  useEffect(() => {
    if (currentUser) {
      history.push(PAGES.CONTINOUS_SINGLE_VIEW);
    }
  }, [currentUser]);
}
