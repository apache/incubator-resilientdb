'use client';

import { TextAnimate } from '@gfazioli/mantine-text-animate';
import { IconBrandGithub, IconExternalLink } from '@tabler/icons-react';
import { Anchor, Button, Center, Paper, Text, Title } from '@mantine/core';
import pack from '../../package.json';
import classes from './Welcome.module.css';

export function Welcome() {
  return (
    <>
      <Title maw="90vw" mx="auto" ta="center" mt={100}>
        <span className={classes.title}>Next Gen Docs for</span>
        <TextAnimate
          animate="in"
          by="character"
          inherit
          variant="gradient"
          component="span"
          segmentDelay={0.2}
          className={classes.subtitle}
          duration={2}
          animation="scale"
          animateProps={{
            scaleAmount: 3,
          }}
          gradient={{ from: 'pink', to: 'yellow' }}
        >
          NexRes
        </TextAnimate>
      </Title>

      <Text c="dimmed" ta="center" size="xl" maw={580} mx="auto" mt="sm">
        Find all documentation related to ResilientDB, its applications, and ecosystem tools
        supported by the ResilientDB team. This site is your gateway to high-performance blockchain
        infrastructure, developer guides, and integration resources. To learn more about
        ResilientDB, visit <Anchor href="https://resilientdb.com/">the official website</Anchor>.
      </Text>

      <Center>
        <Button
          href="https://resilientdb.incubator.apache.org/"
          component="a"
          rightSection={<IconExternalLink />}
          leftSection={<IconBrandGithub />}
          variant="outline"
          px={32}
          radius={256}
          size="lg"
          mx="auto"
          mt="xl"
        >
          Latest Release v{pack.version}
        </Button>
      </Center>

      <Paper shadow="xl" p={8} mih={300} my={32} bg="dark.9" mx="auto" radius={8}>
        <TextAnimate.Typewriter
          inherit
          fz={11}
          c="green.5"
          ff="monospace"
          multiline
          delay={100}
          loop={false}
          value={[
            '🔗 ResilientDB Node Dependencies:',
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
