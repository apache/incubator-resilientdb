"use client";

import { MainNavBar } from '@/components/ui/main-navbar';
import { ReactNode } from 'react';
import "./globals.css";

export default function RootLayout({
  children,
}: {
  children: ReactNode;
}) {
  return (
    <html lang="en">
      <body className="p-0 dark bg-background text-white font-sans overflow-x-hidden">
        <MainNavBar />
        
        <main className="px-4 w-full">
          {children}
        </main>
      </body>
    </html>
  );
}
