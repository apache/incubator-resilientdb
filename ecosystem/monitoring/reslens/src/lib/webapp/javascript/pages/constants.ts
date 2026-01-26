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

export enum PAGES {
  CONTINOUS_SINGLE_VIEW = '/',
  COMPARISON_VIEW = '/comparison',
  COMPARISON_DIFF_VIEW = '/comparison-diff',
  SETTINGS = '/settings',
  LOGIN = '/login',
  SIGNUP = '/signup',
  SERVICE_DISCOVERY = '/service-discovery',
  ADHOC_SINGLE = '/adhoc-single',
  ADHOC_COMPARISON = '/adhoc-comparison',
  ADHOC_COMPARISON_DIFF = '/adhoc-comparison-diff',
  FORBIDDEN = '/forbidden',
  TAG_EXPLORER = '/explore',
  TRACING_EXEMPLARS_MERGE = '/exemplars/merge',
  TRACING_EXEMPLARS_SINGLE = '/exemplars/single',
}

export default {
  PAGES,
};
