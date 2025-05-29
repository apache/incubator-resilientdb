import { useSearchParams } from 'next/navigation';
import { Container, Paper, Text, Title } from '@mantine/core';
import { AuthProviders } from '@/components/AuthProviders';

function ErrorContent() {
  const searchParams = useSearchParams();
  const error = searchParams?.get('error');

  return (
    <Container size="xs" mt={40}>
      <Paper radius="md" p="xl" withBorder>
        <Title order={2} ta="center" mt="md" mb={30} c="red">
          Authentication Error
        </Title>
        <Text size="sm" ta="center">
          {error === 'OAuthSignin'
            ? 'An error occurred during sign in. Please try again.'
            : error || 'An unknown error occurred'}
        </Text>
      </Paper>
    </Container>
  );
}

export default function Error() {
  return (
    <AuthProviders>
      <ErrorContent />
    </AuthProviders>
  );
} 