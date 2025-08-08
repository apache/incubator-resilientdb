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

import {
  Anchor,
  Box,
  Container,
  SimpleGrid,
  Stack,
  Group,
  Text,
  Divider,
  ActionIcon,
  Title,
} from '@mantine/core';
import { IconBrandGithub, IconWorldWww, IconMail } from '@tabler/icons-react';

/**
 * You can customize the Nextra Footer component.
 * Don't forget to use the MantineProvider component.
 *
 * @since 1.0.0
 *
 */
export const MantineFooter = () => (
  <Box component="footer" style={{ position: 'relative' }}>
    {/* Top section */}
    <Box
      py="xl"
      style={{
        background:
          'linear-gradient(180deg, rgba(120, 120, 140, 0.10) 0%, rgba(120, 120, 140, 0.00) 100%)',
        borderTop: '1px solid rgba(127, 127, 127, 0.15)',
      }}
    >
      <Container size="lg">
        <SimpleGrid cols={{ base: 1, sm: 2, md: 3 }} spacing="xl">
          <Stack gap="xs">
            <Title order={6}>Docs</Title>
            <Anchor href="/docs">Overview</Anchor>
            <Anchor href="/docs/installation">Installation</Anchor>
            <Anchor href="/docs/resilientdb">ResilientDB</Anchor>
          </Stack>

          <Stack gap="xs">
            <Title order={6}>Ecosystem</Title>
            <Anchor href="/docs/resilientdb_graphql">ResilientDB GraphQL</Anchor>
            <Anchor href="/docs/reslens">ResLens – Monitoring</Anchor>
            <Anchor href="/docs/resvault">ResVault</Anchor>
          </Stack>

          <Stack gap="xs">
            <Title order={6}>Community</Title>
            <Group gap="xs">
              <ActionIcon component="a" href="https://github.com/apache" target="_blank" aria-label="GitHub">
                <IconBrandGithub size={18} />
              </ActionIcon>
              <ActionIcon component="a" href="https://resilientdb.com" target="_blank" aria-label="Website">
                <IconWorldWww size={18} />
              </ActionIcon>
              <ActionIcon component="a" href="mailto:contact@resilientdb.com" aria-label="Email">
                <IconMail size={18} />
              </ActionIcon>
            </Group>
            <Text size="sm" c="dimmed">
              Docs and tools to help you build with ResilientDB.
            </Text>
          </Stack>
        </SimpleGrid>
      </Container>
    </Box>

    <Divider opacity={0.5} />

    {/* Legal line */}
    <Box py="md">
      <Container size="lg">
        <Text size="sm" c="dimmed" ta="center">
          © {new Date().getFullYear()} ResilientDB. Part of the{' '}
          <Anchor href="https://resilientdb.incubator.apache.org/">Apache Software Foundation</Anchor>.
        </Text>
      </Container>
    </Box>
  </Box>
);