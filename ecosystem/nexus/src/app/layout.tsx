"use client";



import { MainNavBar } from "@/components/ui/main-navbar"; // eslint-disable-line @typescript-eslint/no-unused-vars
import { QueryProvider } from "@/components/providers/QueryProvider";
import { ReactNode } from "react";
import "./globals.css";

export default function RootLayout({ children }: { children: ReactNode }) {
  return (
    <html lang="en">
      <body className="p-0 dark bg-background text-white font-sans overflow-x-hidden">
        <QueryProvider>
          {/* <MainNavBar /> */}
          <main className="w-full">{children}</main>
        </QueryProvider>
      </body>
    </html>
  );
}
