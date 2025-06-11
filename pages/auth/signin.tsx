import { useSearchParams } from 'next/navigation';
import { IconBrandGithub } from '@tabler/icons-react';
import { signIn } from 'next-auth/react';
import { Button, Container, Paper, Stack, Text, Title } from '@mantine/core';
import { AuthProviders } from '@/components/AuthProviders';

function SignInContent() {
  const searchParams = useSearchParams();
  const callbackUrl = searchParams?.get('callbackUrl') || '/';
  const error = searchParams?.get('error');

  const handleGitHubSignIn = () => {
    signIn('github', { callbackUrl });
  };

  return (
    <Container size="xs" mt={40}>
      <Paper radius="md" p="xl" withBorder>
        <Title order={2} ta="center" mt="md" mb={50}>
          Welcome to ResilientDB Documentation
        </Title>

        {error && (
          <Text c="red" size="sm" mb="md" ta="center">
            {error === 'OAuthSignin' ? 'An error occurred during sign in.' : error}
          </Text>
        )}

        <Button
          leftSection={<IconBrandGithub size={20} />}
          variant="default"
          color="gray"
          fullWidth
          onClick={handleGitHubSignIn}
        >
          Continue with GitHub
        </Button>

        <Text c="dimmed" size="xs" ta="center" mt={20}>
          By continuing, you agree to our Terms of Service and Privacy Policy.
        </Text>
      </Paper>
    </Container>
  );
}

export default function SignIn() {
  return (
    <AuthProviders>
      <SignInContent />
    </AuthProviders>
  );
}
