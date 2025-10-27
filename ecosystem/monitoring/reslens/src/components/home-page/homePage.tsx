/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
*/

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
