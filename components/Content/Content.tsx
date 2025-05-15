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
