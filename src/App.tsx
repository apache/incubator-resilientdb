import { Main, Section, Container } from "@/components/craft";
import Description from "@/components/home-page/description";
import "./App.css";
import PBFT from "@/components/home-page/PBFT";
import Graphs from "@/components/home-page/Graphs";
import Cta from "@/components/home-page/CTA";
import { EvervaultCardDemo } from "@/components/home-page/hero";

import { ThemeProvider } from "@/components/theme-provider";

function App() {
  return (
    <ThemeProvider defaultTheme="dark" storageKey="vite-ui-theme">
      <Main className="px-0 py-0">
        <EvervaultCardDemo />
        <Section>
          <Container>
            <Description />
            <PBFT />
            <Graphs />
            <Cta />
          </Container>
        </Section>
      </Main>
    </ThemeProvider>
  );
}

export default App;
