'use client'

import { Button } from '@mantine/core';
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
    <Button
      onClick={handleSearch}
      leftSection={<IconSearch size={20} />}
      variant="subtle"
    >
      Search
    </Button>
  );
} 