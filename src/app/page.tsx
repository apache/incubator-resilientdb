import { Main, Section, Container, Box } from "@/components/craft";
import Description from "@/components/home-page/description";
//import { EvervaultCardDemo } from "/Users/bisman/Documents/MemView FE/MemLens/components/home-page/hero"
//import EvervaultCardDemo from "@/components/home-page/hero";
//import { HeroSection } from "/Users/bisman/Documents/MemView FE/MemLens/components/home-page/hero"
import PBFT from "@/components/home-page/PBFT";
import Graphs from "@/components/home-page/Graphs";
import Cta from "@/components/home-page/CTA";
import { EvervaultCardDemo } from "@/components/home-page/hero";


export default function Home() {
  return (
    <Main className="px-0 py-0">
     <EvervaultCardDemo/>
      <Section>
        <Container>
          <Description />
          <PBFT />
          <Graphs />
          <Cta />
        </Container>
      </Section>
    </Main>
  );
}

