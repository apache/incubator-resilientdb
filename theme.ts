'use client';

import { createTheme } from '@mantine/core';

export const theme = createTheme({
  /* Sleek, modern theme overrides */
  primaryColor: 'blue',
  defaultRadius: 12,
  fontFamily: 'Inter, ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, Noto Sans, \"Apple Color Emoji\", \"Segoe UI Emoji\"',
  headings: {
    fontFamily:
      'Inter, ui-sans-serif, system-ui, -apple-system, Segoe UI, Roboto, Helvetica, Arial, Noto Sans, \"Apple Color Emoji\", \"Segoe UI Emoji\"',
    sizes: {
      h1: { fontSize: '2.75rem', fontWeight: '800', lineHeight: '1.1' },
      h2: { fontSize: '2.25rem', fontWeight: '800', lineHeight: '1.15' },
      h3: { fontSize: '1.75rem', fontWeight: '700', lineHeight: '1.2' },
    },
  },
  colors: {
    blue: [
      '#e8f1ff',
      '#d3e4ff',
      '#a7c6ff',
      '#7aa8ff',
      '#4d8aff',
      '#256dff',
      '#0d5bff',
      '#0046d8',
      '#0037a6',
      '#002874',
    ],
    dark: [
      '#f8f9fa',
      '#f1f3f5',
      '#e9ecef',
      '#dee2e6',
      '#ced4da',
      '#343a40',
      '#2b3035',
      '#212529',
      '#161a1d',
      '#0b0d0e',
    ],
    gray: [
      '#f8fafc',
      '#f1f5f9',
      '#e2e8f0',
      '#cbd5e1',
      '#94a3b8',
      '#64748b',
      '#475569',
      '#334155',
      '#1f2937',
      '#0f172a',
    ],
  },
  components: {
    Button: {
      defaultProps: { radius: 'xl' },
      styles: {
        root: { fontWeight: 600 },
      },
    },
    Paper: {
      defaultProps: { radius: 'lg' },
    },
    Card: {
      defaultProps: { radius: 'lg' },
    },
    Tooltip: {
      defaultProps: { radius: 'md' },
    },
  },
});
