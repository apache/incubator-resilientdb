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

import { z } from 'zod';

const featuresSchema = z.object({
  enableAdhocUI: z.boolean().default(false),
  googleEnabled: z.boolean().default(false),
  gitlabEnabled: z.boolean().default(false),
  githubEnabled: z.boolean().default(false),
  internalAuthEnabled: z.boolean().default(false),
  signupEnabled: z.boolean().default(false),
  isAuthRequired: z.boolean().default(false),
  exportToFlamegraphDotComEnabled: z.boolean().default(false),
  exemplarsPageEnabled: z.boolean().default(false),
});

function hasFeatures(
  window: unknown
): window is typeof window & { features: unknown } {
  if (typeof window === 'object') {
    if (window && window.hasOwnProperty('features')) {
      return true;
    }
  }

  return false;
}

// Parse features at startup
const features = featuresSchema.parse(
  hasFeatures(window) ? window.features : {}
);

// Re-export with more friendly names
export const isAdhocUIEnabled = features.enableAdhocUI;
export const isGoogleEnabled = features.googleEnabled;
export const isGitlabEnabled = features.gitlabEnabled;
export const isGithubEnabled = features.githubEnabled;
export const isInternalAuthEnabled = features.internalAuthEnabled;
export const isSignupEnabled = features.signupEnabled;
export const isExportToFlamegraphDotComEnabled =
  features.exportToFlamegraphDotComEnabled;
export const isAuthRequired = features.isAuthRequired;
export const isExemplarsPageEnabled = features.exemplarsPageEnabled;

// oss only features
export const isAnnotationsEnabled = true;
