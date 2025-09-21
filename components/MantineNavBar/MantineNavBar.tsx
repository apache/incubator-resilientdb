'use client';

import { Box } from '@mantine/core';
import { usePathname } from 'next/navigation';

import '@mantine/core/styles.css';
import { MantineNextraThemeObserver } from '../MantineNextraThemeObserver/MantineNextraThemeObserver';
import Header from '@/components/landing/Header';

/**
 * You can customize the Nextra NavBar component.
 * Don't forget to use the MantineProvider and MantineNextraThemeObserver components.
 *
 * @since 1.0.0
 *
 */
export const MantineNavBar = () => {
  const pathname = usePathname();
  if (pathname === '/') return null;
  return (
    <>
      <MantineNextraThemeObserver />
      <Box>
        <Header />
      </Box>
    </>
  );
};
