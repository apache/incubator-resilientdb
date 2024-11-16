// React and Next.js imports
import Link from "next/link";
import Image from "next/image";

// Third-party library imports
import Balancer from "react-wrap-balancer";

// UI component imports
import { Section, Container } from "@/components/craft";
import { Button } from "@/components/ui/button";

const PBFT = () => {
  return (
    <Section>
      <Container className="grid items-stretch">
        <div className="not-prose relative flex h-96 overflow-hidden rounded-lg border">
          <Image
            src="/placeholder.jpg"
            alt="placeholder"
            className="fill object-cover"
            width={900} // Update as per your image size
            height={300}
          />
        </div>
        <h3>PBFT Graph goes here</h3>
        <p className="text-muted-foreground">
          <Balancer>
          This graph illustrates the message transmission process in PBFT with four nodes, highlighting the request, prepare, and commit phases to achieve consensus and ensure transaction validity.
          </Balancer>
        </p>
        <div className="not-prose mt-8 flex items-center gap-2">
          <Button className="w-fit" asChild>
            <Link href="#">Get Started</Link>
          </Button>
          <Button className="w-fit" variant="link" asChild>
            <Link href="#">Learn More {"->"}</Link>
          </Button>
        </div>
      </Container>
    </Section>
  );
};

export default PBFT;
