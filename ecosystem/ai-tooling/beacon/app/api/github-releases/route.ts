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

export async function GET() {
  const response = await fetch(
    'https://api.github.com/repos/gfazioli/next-app-nextra-template/releases',
    {
      headers: {
        Accept: 'application/vnd.github+json',
        // Authorization: `Bearer ${process.env.GITHUB_TOKEN}`, // Optional for rate limit
      },
    }
  );
  if (!response.ok) {
    return Response.json({ error: 'Failed to fetch releases' }, { status: 500 });
  }
  const releases = await response.json();

  return Response.json({ releases, status: 'ok' });
}
