'use client';

import { useEffect, useState } from 'react';
import { compileMdx } from 'nextra/compile';
import { MDXRemote } from 'nextra/mdx-remote';
import useSWR from 'swr';
import { Alert, Badge, Group, Loader, Paper, Skeleton, Stack, Text } from '@mantine/core';
import { useMDXComponents } from '@/mdx-components';

interface Author {
  login: string;
  id: number;
  node_id: string;
  avatar_url: string;
  gravatar_id: string;
  url: string;
  html_url: string;
  followers_url: string;
  following_url: string;
  gists_url: string;
  starred_url: string;
  subscriptions_url: string;
  organizations_url: string;
  repos_url: string;
  events_url: string;
  received_events_url: string;
  type: string;
  user_view_type: string;
  site_admin: boolean;
}

interface Release {
  url: string;
  assets_url: string;
  upload_url: string;
  html_url: string;
  id: number;
  author: Author;
  node_id: string;
  tag_name: string;
  target_commitish: string;
  name: string;
  draft: boolean;
  prerelease: boolean;
  created_at: string;
  published_at: string;
  assets: any[];
  tarball_url: string;
  zipball_url: string;
  body: string;
}

const fetcher = (url: string) => fetch(url).then((res) => res.json());

export function ReleaseNotes() {
  const { data, error, isLoading } = useSWR<{
    releases: Release[];
  }>('/api/github-releases', fetcher);
  const [compiledReleases, setCompiledReleases] = useState<Release[]>([]);

  const components = useMDXComponents();

  useEffect(() => {
    if (data) {
      const fetchReleases = async () => {
        const releases = await Promise.all(
          data.releases.map(async (release) => ({
            ...release,
            created_at: new Date(release.created_at).toLocaleDateString('en-US', {
              year: 'numeric',
              month: 'long',
              day: 'numeric',
            }),
            body: await compileMdx(release.body),
          }))
        );
        setCompiledReleases(releases);
      };
      fetchReleases();
    }
  }, [data, isLoading, error]); // Add isLoading and error to the dependency array

  if (isLoading || compiledReleases.length === 0) {
    return (
      <Stack mt={24} w="100%" align="center">
        <Group>
          <Text>Loading releases...</Text>
          <Loader type="dots" />
        </Group>
        <Skeleton height={200} width="100%" radius={12} />
        <Skeleton height={50} width="100%" radius={12} />
        <Skeleton height={20} width="100%" radius={12} />
      </Stack>
    );
  }

  if (error) {
    return (
      <Alert title="Error" color="red">
        Failed to load releases
      </Alert>
    );
  }

  if (!data) {
    return null;
  }

  if (compiledReleases.length === 0) {
    return null;
  }

  return (
    <Stack mt={24}>
      {compiledReleases.map((release: Release) => (
        <Paper key={release.id} withBorder p={24} radius={12} shadow="sm">
          <Group justify="space-between">
            <Badge component="a" href={release.html_url} size="xl">
              {release.name}
            </Badge>
            <Text>{release.created_at}</Text>
          </Group>

          <MDXRemote compiledSource={release.body} components={components} />
        </Paper>
      ))}
    </Stack>
  );
}
