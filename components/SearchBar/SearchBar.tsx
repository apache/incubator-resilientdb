'use client'

import { Button, Group, Kbd, Text } from '@mantine/core';
import { IconSearch } from '@tabler/icons-react';

export function SearchBar() {
  const handleSearch = () => {
    // This triggers Nextra's built-in search using keyboard shortcut
    const event = new KeyboardEvent('keydown', {
      key: 'k',
      metaKey: true, // For Mac
      ctrlKey: true, // For Windows/Linux
    });
    document.dispatchEvent(event);
  };

  return (
    <Button onClick={handleSearch} variant="subtle" radius="xl" px={12}>
      <Group gap={6} wrap="nowrap">
        <IconSearch size={18} />
        <Text size="sm" c="dimmed">Search</Text>
        <Kbd>Ctrl</Kbd>
        <Text size="sm" c="dimmed">+</Text>
        <Kbd>K</Kbd>
      </Group>
    </Button>
  );
} 