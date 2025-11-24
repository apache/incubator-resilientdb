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

import { MantineFooter, MantineNavBar } from '@/components';

import '@gfazioli/mantine-marquee/styles.layer.css';
import '@gfazioli/mantine-text-animate/styles.layer.css';

import { ColorSchemeScript, mantineHtmlProps, MantineProvider } from '@mantine/core';

import { Layout } from 'nextra-theme-docs';
import { Banner, Head } from 'nextra/components';
import { getPageMap } from 'nextra/page-map';
import { theme } from '../theme';
import { Inter } from 'next/font/google';

import './global.css';

import { FloatingAssistant } from '@/components/FloatingAssistant';
import { PocketFlowChatbot } from '@/components/PocketFlowChatbot';
import { Providers } from './providers';

const inter = Inter({ subsets: ['latin'], variable: '--font-sans' });

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
    <html lang="en" dir="ltr" {...mantineHtmlProps} className={`${inter.className} ${inter.variable}`}>
      <Head>
        <ColorSchemeScript nonce="8IBTHwOdqNKAWeKl7plt8g==" defaultColorScheme="dark" />
        <link rel="shortcut icon" href="/favicon.svg" type="image/svg+xml" />
        <meta
          name="viewport"
          content="minimum-scale=1, initial-scale=1, width=device-width, user-scalable=no"
        />
      </Head>
      <body>
        <Providers>
          <MantineProvider theme={theme} defaultColorScheme="dark">
            <Layout
              // banner={
              //   <Banner storageKey="mantine-nextjs-nextra">
              //     ✨ AI powered documentation for ResilientDB Ecosystem and Apps
              //   </Banner>
              // }
              navbar={<MantineNavBar />}
              pageMap={pageMap}
              docsRepositoryBase="https://github.com/shuding/nextra/tree/main/docs"
              footer={<MantineFooter />}
              sidebar={{ defaultMenuCollapseLevel: 1, toggleButton: true }}
            >
              {children}
            </Layout>
            <FloatingAssistant />
            {/* <PocketFlowChatbot /> */}
          </MantineProvider>
        </Providers>
      </body>
    </html>
  );
}
