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

'use client';

import { useState } from 'react';
import { IconAlertCircle } from '@tabler/icons-react';
import { Alert, Button, Paper, Stack, Text, Textarea } from '@mantine/core';
import { GitHubAuth } from './GitHubAuth';

interface GitHubUser {
  login: string;
  avatar_url: string;
  name: string | null;
}

interface CommentSectionProps {
  pageTitle: string;
  pageUrl: string;
  repoOwner: string;
  repoName: string;
  labels?: string[];
  title?: string;
}

export function CommentSection({
  pageTitle,
  pageUrl,
  repoOwner,
  repoName,
  labels = ['user-feedback', 'documentation'],
  title = 'Leave a Comment',
}: CommentSectionProps) {
  const [comment, setComment] = useState('');
  const [isSubmitting, setIsSubmitting] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [success, setSuccess] = useState(false);
  const [user, setUser] = useState<GitHubUser | null>(null);

  const handleSubmit = async () => {
    if (!user) {
      setError('Please sign in with GitHub to submit a comment');
      return;
    }

    try {
      setIsSubmitting(true);
      setError(null);

      const response = await fetch('/api/create-github-issue', {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({
          title: `Feedback: ${pageTitle}`,
          body: comment,
          repoOwner,
          repoName,
          labels,
        }),
      });

      if (!response.ok) {
        const errorData = await response.json();
        throw new Error(errorData.error || 'Failed to create GitHub issue');
      }

      setSuccess(true);
      setComment('');
    } catch (err) {
      setError(
        err instanceof Error ? err.message : 'Failed to submit comment. Please try again later.'
      );
      console.error('Error creating GitHub issue:', err);
    } finally {
      setIsSubmitting(false);
    }
  };

  return (
    <Paper p="md" withBorder>
      <Stack gap="md">
        <Text fw={500} size="lg">
          {title}
        </Text>
        <GitHubAuth onAuthStateChange={setUser} returnUrl={pageUrl} />
        {user && (
          <>
            <Textarea
              required
              label="Comment"
              value={comment}
              onChange={(event) => setComment(event.currentTarget.value)}
              placeholder="Share your thoughts, suggestions, or report issues..."
              minRows={3}
              disabled={isSubmitting}
            />
            {error && (
              <Alert icon={<IconAlertCircle size={16} />} color="red">
                {error}
              </Alert>
            )}
            {success && (
              <Alert color="green">
                Thank you for your feedback! We've created an issue and our team will review it.
              </Alert>
            )}
            <Button onClick={handleSubmit} loading={isSubmitting} disabled={!comment.trim()}>
              Submit Comment
            </Button>
          </>
        )}
      </Stack>
    </Paper>
  );
}
