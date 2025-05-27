'use client';

import { Group, Text } from '@mantine/core';

import '@mantine/core/styles.css';

import { Navbar } from 'nextra-theme-docs';
import { ColorSchemeControl } from '../ColorSchemeControl/ColorSchemeControl';
import { Logo } from '../Logo/Logo';
import { MantineNextraThemeObserver } from '../MantineNextraThemeObserver/MantineNextraThemeObserver';
import { AskAIModal } from '../AskAIModal';
import { SearchBar } from '../SearchBar/SearchBar';

/**
 * You can customize the Nextra NavBar component.
 * Don't forget to use the MantineProvider and MantineNextraThemeObserver components.
 *
 * @since 1.0.0
 *
 */
export const MantineNavBar = () => {
  return (
    <>
      <MantineNextraThemeObserver />
      <Navbar
        logo={
          <Group align="center" gap={4}>
            <Logo />
            <Text size="lg" fw={800} c="blue" visibleFrom="xl" style={{marginLeft: 2}}>
              Beacon
            </Text>
          </Group>
        }
      >
        <>
          <Group gap="sm">
            {/* <AskAIModal /> */}
            <ColorSchemeControl />
          </Group>
        </>
      </Navbar>
    </>
  );
};
