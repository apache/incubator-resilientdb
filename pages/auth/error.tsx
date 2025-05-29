'use client';

import { Container, Paper, Stack, Text, Title } from '@mantine/core';

export default function Error() {
  return (
    <Container size="xs" py="xl">
      <Paper radius="md" p="xl" withBorder>
        <Stack gap="md">
          <Title order={2}>Authentication Error</Title>
          <Text c="dimmed" size="sm" ta="center">
            There was an error during authentication. Please try again or contact support if the issue persists.
          </Text>
        </Stack>
      </Paper>
    </Container>
  );
} 