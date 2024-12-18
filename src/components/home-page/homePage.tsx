import { Main, Section, Container } from "@/components/craft";
import Description from "@/components/home-page/description";
import Navbar from "../ui/navbar";
import { GoogleGeminiEffectDemo } from "./hero";
import Footer from "../ui/footer";
import PBFTVisualization from "../ui/PBFT graph/PBFTVisualization";
import { GlowingStarsBackgroundCardPreview } from "../ui/starcardcta";

export function HomePage() {
  return (
    <Main className="px-0 py-0">
      <Navbar />
      <GoogleGeminiEffectDemo />
      <Section>
        <Container>
          <Description />
          <PBFTVisualization />
          {/* <div className="bg-[#1a1a1a] min-h-screen mt-14">
          {/* Your existing content, including the PBFT consensus visualization
          <InteractiveCharts />
          </div> */}
          <GlowingStarsBackgroundCardPreview />
        </Container>
      </Section>
      <Footer />
    </Main>
  );
}
