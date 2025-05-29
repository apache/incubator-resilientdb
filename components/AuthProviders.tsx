'use client';

import { MantineProvider } from '@mantine/core';
import { SessionProvider } from 'next-auth/react';
import { theme } from '../theme';

export function AuthProviders({ children }: { children: React.ReactNode }) {
  return (
    <SessionProvider>
      <MantineProvider theme={theme}>
        {children}
      </MantineProvider>
    </SessionProvider>
  );
} 