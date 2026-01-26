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

import React, { useContext } from 'react';
import { Helmet } from 'react-helmet';

const defaultAppName = 'Pyroscope';
export const AppNameContext = React.createContext(defaultAppName);

function getFullTitle(title: string, appName: string) {
  const finalAppName = appName || defaultAppName;

  return `${title} | ${finalAppName}`;
}

interface PageTitleProps {
  /** Title of the page */
  title: string;
}

/**
 * PageTitleWithAppName will add a page name suffix from the context
 */
export default function PageTitleWithAppName({ title }: PageTitleProps) {
  const appName = useContext(AppNameContext);
  const fullTitle = getFullTitle(title, appName);

  return (
    <Helmet>
      <title>{fullTitle}</title>
    </Helmet>
  );
}
