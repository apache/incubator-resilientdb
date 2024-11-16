import { Main, Section, Container, Box } from "@/components/craft";
import Description from "@/components/home-page/description";
import Hero from "@/components/home-page/hero";
import PBFT from "@/components/home-page/PBFT";
import Graphs from "@/components/home-page/Graphs";

export default function Home() {
  return (
    <Main>
    <Section>
      <Container>
        <Hero/>
        <Description/>
        <PBFT/>
        <Graphs/>
      </Container>
    </Section>
  </Main>
  );
}
