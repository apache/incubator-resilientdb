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

'use client';

import { TextAnimate } from '@gfazioli/mantine-text-animate';
import { IconBrandGithub, IconExternalLink } from '@tabler/icons-react';
import { Anchor, Button, Center, Paper, Text, Title, Stack } from '@mantine/core';
import pack from '../../package.json';
import classes from './Welcome.module.css';

export function Welcome() {
  return (
    <>
      <div className={classes.hero}>
        <div className={classes.halo} />
        <Stack gap="sm" align="center" mt={64}>
          <Title maw="900px" mx="auto" ta="center">
            <span className={classes.title}>Next‑gen docs for </span>
            <Text component="span" variant="gradient" gradient={{ from: 'pink', to: 'yellow' }} className={classes.subtitle}>
              NexRes
            </Text>
          </Title>

          <Text c="dimmed" ta="center" size="lg" maw={720} mx="auto">
            Find all documentation related to ResilientDB, its applications, and ecosystem tools
            supported by the ResilientDB team. This site is your gateway to high-performance
            blockchain infrastructure, developer guides, and integration resources. To learn more
            about ResilientDB, visit <Anchor href="https://resilientdb.com/">the official website</Anchor>.
          </Text>

          <Center>
            <Button
              href="https://resilientdb.incubator.apache.org/"
              component="a"
              rightSection={<IconExternalLink />}
              leftSection={<IconBrandGithub />}
              variant="gradient"
              gradient={{ from: 'blue', to: 'grape' }}
              px={28}
              radius={999}
              size="md"
              mx="auto"
              mt="sm"
            >
              Latest Release v{pack.version}
            </Button>
          </Center>
        </Stack>
      </div>

      <Paper shadow="xl" p={8} mih={300} my={32} bg="dark.9" mx="auto" radius={12}>
        <TextAnimate.Typewriter
          inherit
          fz={11}
          c="green.5"
          ff="monospace"
          multiline
          delay={100}
          loop={false}
          value={[
            'Dependencies:',
            ...Object.keys(pack.dependencies).map(
              (key: string) =>
                `• ${key} : ${pack.dependencies[key as keyof typeof pack.dependencies].toString()}`
            ),
            '',
            'Each dependency is a block in your development chain.',
            'Keep your stack resilient!',
          ]}
        />
      </Paper>
    </>
  );
}
