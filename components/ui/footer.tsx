import Image from "next/image";
import Link from "next/link";
import Balancer from "react-wrap-balancer";

import { Section, Container } from "../craft";
import Logo from "@/public/logo.svg";

export default function Footer() {
  return (
    <footer className="not-prose border-t">
      <Section>
        <Container className="grid gap-6">
          <div className="grid gap-4">
            <Link href="/">
              <h3 className="sr-only">brijr/components</h3>
              <Image
                src="/logo.svg"
                alt="Logo"
                width={120}
                height={27.27}
                className="transition-all hover:opacity-75 dark:invert"
              ></Image>
            </Link>
            <p>
              <Balancer>
                
The MemLens project analyzes ResilientDB’s performance to enhance efficiency on resource-limited devices.
              </Balancer>
            </p>
            <div className="mb-6 flex flex-col gap-4 text-sm text-muted-foreground underline underline-offset-4 md:mb-0 md:flex-row">
              <Link href="/privacy-policy">Privacy Policy</Link>
              <Link href="/terms-of-service">Terms of Service</Link>
              <Link href="/cookie-policy">Cookie Policy</Link>
            </div>
            <p className="text-muted-foreground">
              ©{" "}
              <a href="https://github.com/brijr/components">MemLens</a>
              . All rights reserved. 2024-Present.
            </p>
          </div>
        </Container>
      </Section>
    </footer>
  );
}
