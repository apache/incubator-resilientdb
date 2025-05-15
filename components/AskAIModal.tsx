'use client'

import { Button, Modal, TextInput, Stack, Text, Divider } from '@mantine/core';
import { useDisclosure } from '@mantine/hooks';
import { IconRobot, IconSearch } from '@tabler/icons-react';
import { useRouter } from 'next/navigation';

export function AskAIModal() {
  const [opened, { open, close }] = useDisclosure(false);
  const router = useRouter();

  const handleSearch = (query: string) => {
    // This will navigate to the search page with the query
    if (query) {
      router.push(`/?q=${encodeURIComponent(query)}`);
      close();
    }
  };

  return (
    <>
      <Button 
        onClick={open}
        leftSection={<IconRobot size={20} />}
        variant="subtle"
      >
        Ask AI
      </Button>

      <Modal 
        opened={opened} 
        onClose={close} 
        title="Ask AI Assistant" 
        size="lg"
      >
        <Stack>
          <Text size="sm" c="dimmed">
            Ask me anything about ResilientDB and its ecosystem. I'm here to help!
          </Text>
          <TextInput
            placeholder="Ask the AI assistant..."
            size="md"
            leftSection={<IconRobot size={16} />}
            onKeyDown={(e) => {
              if (e.key === 'Enter') {
                handleSearch((e.target as HTMLInputElement).value);
              }
            }}
          />
          <Text size="xs" c="dimmed">
            Coming soon: Integration with ResilientDB's AI assistant for more intelligent responses.
          </Text>

          <Divider label="Or search the documentation" labelPosition="center" my="md" />

          <Text size="sm" c="dimmed">
            Search through our comprehensive documentation to find detailed information.
          </Text>
          <TextInput
            placeholder="Search documentation..."
            size="md"
            leftSection={<IconSearch size={16} />}
            onKeyDown={(e) => {
              if (e.key === 'Enter') {
                handleSearch((e.target as HTMLInputElement).value);
              }
            }}
          />
          <Text size="xs" c="dimmed">
            Press Enter to search through the entire documentation.
          </Text>
        </Stack>
      </Modal>
    </>
  );
} 