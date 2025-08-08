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

import { NextApiRequest, NextApiResponse } from 'next';
import { Octokit } from '@octokit/rest';
import { getServerSession } from 'next-auth';
import { authOptions } from './auth/[...nextauth]';

// Rate limiting map (in production you'd want to use Redis or similar)
const requestMap = new Map<string, { count: number; timestamp: number }>();

const RATE_LIMIT = {
  MAX_REQUESTS: process.env.NODE_ENV === 'production' ? 5 : 50, // Stricter in production
  WINDOW_MS: process.env.NODE_ENV === 'production' ? 60000 : 300000, // 1 minute in prod, 5 in dev
};

function isRateLimited(identifier: string): boolean {
  const now = Date.now();
  const userRequests = requestMap.get(identifier);

  if (!userRequests) {
    requestMap.set(identifier, { count: 1, timestamp: now });
    return false;
  }

  if (now - userRequests.timestamp > RATE_LIMIT.WINDOW_MS) {
    requestMap.set(identifier, { count: 1, timestamp: now });
    return false;
  }

  if (userRequests.count >= RATE_LIMIT.MAX_REQUESTS) {
    return true;
  }

  userRequests.count += 1;
  return false;
}

export default async function handler(req: NextApiRequest, res: NextApiResponse) {
  // Only allow POST requests
  if (req.method !== 'POST') {
    return res.status(405).json({ error: 'Method not allowed' });
  }

  try {
    const session = await getServerSession(req, res, authOptions);

    if (!session?.accessToken) {
      return res.status(401).json({ error: 'Unauthorized - Please sign in with GitHub' });
    }

    // Rate limiting check
    const userIdentifier =
      session.user?.email ||
      (Array.isArray(req.headers['x-forwarded-for'])
        ? req.headers['x-forwarded-for'][0]
        : req.headers['x-forwarded-for']) ||
      'anonymous';

    if (isRateLimited(userIdentifier)) {
      return res.status(429).json({
        error: 'Too many requests. Please try again later.',
        retryAfter: RATE_LIMIT.WINDOW_MS / 1000,
      });
    }

    const { title, body, repoOwner, repoName, labels } = req.body;

    // Validate required fields
    if (!title || !body || !repoOwner || !repoName) {
      return res.status(400).json({
        error: 'Title, body, repository owner, and repository name are required',
      });
    }

    // Additional production validations
    if (process.env.NODE_ENV === 'production') {
      if (title.length > 256) {
        return res.status(400).json({ error: 'Title is too long (max 256 characters)' });
      }
      if (body.length > 65536) {
        return res.status(400).json({ error: 'Body is too long (max 65536 characters)' });
      }
    }

    // Initialize Octokit with the user's access token
    const octokit = new Octokit({
      auth: session.accessToken,
    });

    try {
      // Create the issue using GitHub API
      const response = await octokit.issues.create({
        owner: repoOwner,
        repo: repoName,
        title,
        body: `${body}\n\n---\n_Posted via ${process.env.NEXTAUTH_URL}_`,
        labels: labels || ['user-feedback', 'documentation'],
      });

      return res.json({
        success: true,
        issueUrl: response.data.html_url,
        issueNumber: response.data.number,
      });
    } catch (githubError: any) {
      // Handle specific GitHub API errors
      const errorMessage = githubError.response?.data?.message || 'Failed to create GitHub issue';
      return res.status(githubError.status || 500).json({ error: errorMessage });
    }
  } catch (error) {
    console.error('Error processing request:', error);
    return res.status(500).json({ error: 'Internal server error' });
  }
}
