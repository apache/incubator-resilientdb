// React and Next.js imports
// Third-party library imports
import Balancer from "react-wrap-balancer";
import { Camera } from "lucide-react";

// Local component imports
import { Section, Container } from "@/components/craft";
import { Button } from "@/components/ui/button";

// Asset imports
// import Logo from "/logo.svg";

const Hero = () => {
  return (
    <Section>
      <Container className="flex flex-col items-center text-center">
        <image
          href="/logo.svg"
          width={172}
          height={72}
          className="not-prose mb-6 dark:invert md:mb-8"
        />
        <h1 className="text-4xl font-bold mb-4">
          <Balancer>MemLens ResilientDB Performance Profiling</Balancer>
        </h1>
        <h3 className="text-muted-foreground">
          <Balancer>
            Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris
            nisi ut aliquip ex ea commodo consequat.
          </Balancer>
        </h3>
        <div className="not-prose mt-6 flex gap-2 md:mt-12">
          <Button asChild>
            <a href="/">
              <Camera className="mr-2" />
              Lorem Ipsum
            </a>
          </Button>
          <Button variant={"ghost"} asChild>
            <a href="/posts">Dolor Sit Amet -{">"}</a>
          </Button>
        </div>
      </Container>
    </Section>
  );
};

export default Hero;
