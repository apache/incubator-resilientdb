import { MantineFooter, MantineNavBar } from '@/components';

import '@gfazioli/mantine-marquee/styles.layer.css';
import '@gfazioli/mantine-text-animate/styles.layer.css';

import { ColorSchemeScript, mantineHtmlProps, MantineProvider } from '@mantine/core';

import '@mantine/core/styles.layer.css';

import { Layout } from 'nextra-theme-docs';
import { Banner, Head } from 'nextra/components';
import { getPageMap } from 'nextra/page-map';
import { theme } from '../theme';

import './global.css';

import { FloatingAssistant } from '@/components/FloatingAssistant';
import { Providers } from './providers';

export const metadata = {
  title: {
    default: 'Beacon - NextGen Docs for Nexres',
    template: '%s | ResilientDB Documentation',
  },
  description:
    'Official documentation for ResilientDB, a high-performance, scalable blockchain database for mission-critical applications.',
  metadataBase: new URL('https://resilientdb.com/'),
  keywords: [
    'ResilientDB',
    'Blockchain',
    'Database',
    'Distributed Systems',
    'Consensus',
    'PBFT',
    'Next.js',
    'Mantine',
    'Nextra',
    'Documentation',
  ],
  generator: 'Next.js',
  applicationName: 'ResilientDB',
  appleWebApp: {
    title: 'ResilientDB',
  },
  openGraph: {
    url: 'https://resilientdb.com/',
    siteName: 'ResilientDB',
    locale: 'en_US',
    type: 'website',
  },
  other: {
    'msapplication-TileColor': '#fff',
  },
  twitter: {
    site: 'https://resilientdb.com/',
  },
  alternates: {
    canonical: 'https://resilientdb.com/',
  },
};

export default async function RootLayout({ children }: { children: any }) {
  const pageMap = await getPageMap();

  return (
    <html lang="en" dir="ltr" {...mantineHtmlProps}>
      <Head>
        <ColorSchemeScript nonce="8IBTHwOdqNKAWeKl7plt8g==" defaultColorScheme="dark" />
        <link rel="shortcut icon" href="/lighthouse.png" />
        <meta
          name="viewport"
          content="minimum-scale=1, initial-scale=1, width=device-width, user-scalable=no"
        />
      </Head>
      <body>
        <Providers>
          <MantineProvider theme={theme} defaultColorScheme="auto">
            <Layout
              banner={
                <Banner storageKey="mantine-nextjs-nextra">
                  ✨ AI powered documentation for ResilientDB Ecosystem and Apps
                </Banner>
              }
              navbar={<MantineNavBar />}
              pageMap={pageMap}
              docsRepositoryBase="https://github.com/shuding/nextra/tree/main/docs"
              footer={<MantineFooter />}
              sidebar={{ defaultMenuCollapseLevel: 1 }}
            >
              {children}
            </Layout>
            <FloatingAssistant />
          </MantineProvider>
        </Providers>
      </body>
    </html>
  );
}
