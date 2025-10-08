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

'use client'

import { Button, Modal, TextInput, Stack, Text, Divider } from '@mantine/core';
import { useDisclosure } from '@mantine/hooks';
import { useEffect, useRef } from 'react';
import { IconRobot, IconSearch } from '@tabler/icons-react';
import { useRouter } from 'next/navigation';

export function AskAIModal() {
  const [opened, { open: openDisclosure, close: closeDisclosure }] = useDisclosure(false);
  const triggerRef = useRef<HTMLButtonElement | null>(null);
  const firstInputRef = useRef<HTMLInputElement | null>(null);
  const lastFocusedRef = useRef<HTMLElement | null>(null);

  const open = (from?: HTMLElement | null) => {
    lastFocusedRef.current = from ?? (document.activeElement as HTMLElement | null);
    openDisclosure();
  };

  const close = () => {
    closeDisclosure();
    // return focus to last focused element
    setTimeout(() => {
      try {
        lastFocusedRef.current?.focus?.();
      } catch (err) {}
    }, 0);
  };

  // Add Escape handler while modal is open
  useEffect(() => {
    if (!opened) return;
    const onKey = (e: KeyboardEvent) => {
      if (e.key === 'Escape') {
        e.preventDefault();
        close();
      }
    };
    document.addEventListener('keydown', onKey);
    // focus first input when opened
    setTimeout(() => firstInputRef.current?.focus(), 50);
    return () => document.removeEventListener('keydown', onKey);
  }, [opened]);
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
        ref={triggerRef}
        onClick={(e) => open(e.currentTarget)}
        onKeyDown={(e) => {
          if (e.key === 'Enter' || e.key === ' ') {
            e.preventDefault();
            open(e.currentTarget as HTMLElement);
          }
        }}
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
            ref={firstInputRef}
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