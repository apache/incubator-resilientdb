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
          <image
            href="/placeholder.jpg"
            className="fill object-cover"
            width={900} // Update as per your image size
            height={300}
          />
        </div>
        <h3>PBFT Graph goes here</h3>
        <p className="text-muted-foreground">
          <Balancer>
            This graph illustrates the message transmission process in PBFT with
            four nodes, highlighting the request, prepare, and commit phases to
            achieve consensus and ensure transaction validity.
          </Balancer>
        </p>
        <div className="not-prose mt-8 flex items-center gap-2">
          <Button className="w-fit" asChild>
            <a href="#">Get Started</a>
          </Button>
          <Button className="w-fit" variant="link" asChild>
            <a href="#">Learn More {"->"}</a>
          </Button>
        </div>
      </Container>
    </Section>
  );
};

export default PBFT;
