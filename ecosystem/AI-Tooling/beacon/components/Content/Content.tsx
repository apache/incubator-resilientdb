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

import { Marquee } from '@gfazioli/mantine-marquee';
import { Anchor } from 'nextra/components';
import { Button, Divider, Stack, Title } from '@mantine/core';

export const Content = () => {
  return (
    <>
      <Divider my="md" />
      <Stack align="center" my={32}>
        <Title order={1} ta="center">
          Explore Apps and Ecosystem
        </Title>

        <Anchor href="https://mantine-extensions.vercel.app/">
          Visit the ResilientDB Hub for more projects
        </Anchor>

        <Marquee fadeEdges pauseOnHover>
          <Button
            size="xl"
            component="a"
            href="https://github.com/apache/incubator-resilientdb"
            target="_blank"
          >
            ResilientDB
          </Button>
          <Button
            size="xl"
            component="a"
            href="https://github.com/apache/incubator-resilientdb-graphql"
            target="_blank"
          >
            ResilientDB Graphql
          </Button>
          <Button
            size="xl"
            component="a"
            href="https://github.com/apache/incubator-resilientdb-resilient-python-cache"
            target="_blank"
          >
            Resilient-Python-Cache
          </Button>
          <Button
            size="xl"
            component="a"
            href="https://github.com/harish876/ResLens"
            target="_blank"
          >
            ResLens
          </Button>
        </Marquee>
      </Stack>
    </>
  );
};
