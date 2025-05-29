'use client';

import { Button, Container, Paper, Stack, Text, Title } from '@mantine/core';
import { IconBrandGithub } from '@tabler/icons-react';
import { signIn } from 'next-auth/react';
import { useSearchParams } from 'next/navigation';

export default function SignIn() {
  const searchParams = useSearchParams();
  const callbackUrl = searchParams.get('callbackUrl') || '/';
  const error = searchParams.get('error');

  const handleGitHubSignIn = () => {
    signIn('github', { 
      callbackUrl: callbackUrl,
      redirect: true,
    });
  };

  return (
    <Container size="xs" py="xl">
      <Paper radius="md" p="xl" withBorder>
        <Stack gap="md">
          <Title order={2}>Sign in to Comment</Title>
          <Text c="dimmed" size="sm" ta="center">
            Sign in with GitHub to leave comments and create issues
          </Text>
          {error && (
            <Text c="red" size="sm" ta="center">
              There was an error signing in. Please try again.
            </Text>
          )}
          <Button
            onClick={handleGitHubSignIn}
            leftSection={<IconBrandGithub size={16} />}
            variant="default"
            fullWidth
          >
            Continue with GitHub
          </Button>
        </Stack>
      </Paper>
    </Container>
  );
} 