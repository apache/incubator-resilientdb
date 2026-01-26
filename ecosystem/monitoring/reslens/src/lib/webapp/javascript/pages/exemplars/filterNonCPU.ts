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

/**
 * filterNonCPU filters out apps that are not cpu
 * it DOES not filter apps that can't be identified
 * Notice that the heuristic here is weak and should be updated when more info is available (such as units)
 */
export function filterNonCPU(appName: string): boolean {
  const suffix = appName.split('.').pop();
  if (!suffix) {
    return true;
  }

  // Golang
  if (suffix.includes('alloc_objects')) {
    return false;
  }

  if (suffix.includes('alloc_space')) {
    return false;
  }

  if (suffix.includes('goroutines')) {
    return false;
  }

  if (suffix.includes('inuse_objects')) {
    return false;
  }

  if (suffix.includes('inuse_space')) {
    return false;
  }

  if (suffix.includes('mutex_count')) {
    return false;
  }

  if (suffix.includes('mutex_duration')) {
    return false;
  }

  // Java
  if (suffix.includes('alloc_in_new_tlab_bytes')) {
    return false;
  }

  if (suffix.includes('alloc_in_new_tlab_objects')) {
    return false;
  }

  if (suffix.includes('lock_count')) {
    return false;
  }

  if (suffix.includes('lock_duration')) {
    return false;
  }

  return true;
}
