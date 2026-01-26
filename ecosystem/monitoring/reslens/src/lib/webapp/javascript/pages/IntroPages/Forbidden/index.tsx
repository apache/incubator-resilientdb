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

/* eslint css-modules/no-unused-class: [2, { markAsUsed: ['link'] }] */
import React from 'react';
import cx from 'classnames';
import { Link } from 'react-router-dom';
import styles from '../IntroPages.module.css';

function ForbiddenPage() {
  return (
    <div className={styles.loginWrapper}>
      <div className={styles.form}>
        <div className={styles.formHeader}>
          <div className={styles.logo} />
          <h1>Authentication error</h1>
        </div>
        <Link to="/login" className={cx(styles.button, styles.buttonDark)}>
          Go back to login page
        </Link>
      </div>
    </div>
  );
}

export default ForbiddenPage;
