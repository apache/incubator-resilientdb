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

import { useTheme } from 'nextra-theme-docs';
import { useMantineColorScheme } from '@mantine/core';
import { useDidUpdate } from '@mantine/hooks';

/**
 * This component is responsible for observing the theme changes in Nextra and Mantine.
 * By using this component, you can ensure that the Mantine theme is always in sync with the Nextra theme.
 *
 * This component is used in the MantineNavBar component.
 *
 * @since 1.0.0
 *
 * @see https://mantine.dev/docs/color-scheme/
 */
export function MantineNextraThemeObserver() {
  const { setColorScheme } = useMantineColorScheme();
  const { theme } = useTheme();

  useDidUpdate(() => {
    setColorScheme(theme === 'dark' ? 'dark' : theme === 'system' ? 'auto' : 'light');
  }, [theme]);

  return null;
}
