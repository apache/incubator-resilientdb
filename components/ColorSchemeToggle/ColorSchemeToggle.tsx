'use client';

import { useTheme } from 'next-themes';
import { Button, Group, useMantineColorScheme } from '@mantine/core';

export function ColorSchemeToggle() {
  const { setColorScheme } = useMantineColorScheme();
  const { setTheme } = useTheme();

  /**
   * You might improve this component. Anyway, it's a good starting point.
   * As you can see we have to handle both the Mantine and Nextra dark mode.
   */

  return (
    <Group justify="center" mt="xl">
      <Button
        onClick={() => {
          setColorScheme('light');
          setTheme('light');
        }}
      >
        Light
      </Button>
      <Button
        onClick={() => {
          setColorScheme('dark');
          setTheme('dark');
        }}
      >
        Dark
      </Button>
      <Button
        onClick={() => {
          setColorScheme('auto');
          setTheme('system');
        }}
      >
        Auto
      </Button>
    </Group>
  );
}
