import { Main, Section, Container } from "@/components/craft";
import Description from "@/components/home-page/description";
import { Button } from "@/components/ui/button";
import {
  DiscIcon as DiscordIcon,
  GithubIcon,
  BarChart2Icon,
} from "lucide-react";
import Navbar from "../ui/navbar";
import { GoogleGeminiEffectDemo } from "./hero";
import Footer from "../ui/footer";
import PBFTVisualization from "../ui/PBFT graph/PBFTVisualization";
import { GlowingStarsBackgroundCardPreview } from "../ui/starcardcta";
import { InteractiveCharts } from "../HPGraphs/InteractiveCharts";

export function HomePage() {
  return (
    <Main className="px-0 py-0">
      <Navbar />
      <GoogleGeminiEffectDemo />
      <Section>
        <Container>
          <Description />
          <PBFTVisualization />
          <div className="bg-[#1a1a1a] min-h-screen mt-14">
          {/* Your existing content, including the PBFT consensus visualization */}
          <InteractiveCharts />
          </div>
          <GlowingStarsBackgroundCardPreview/>
        </Container>
      </Section>
      <Footer/>
    </Main>
  );
}

export function HomePageV1() {
  return (
    <Main className="px-0 py-0">
      <header className="mx-auto px-4 py-6 flex justify-between items-center relative z-10">
        <div className="flex items-center space-x-4">
          <svg
            className="w-12 h-12"
            viewBox="0 0 48 48"
            fill="none"
            xmlns="http://www.w3.org/2000/svg"
          >
            <circle
              cx="24"
              cy="24"
              r="23"
              fill="#2C3E50"
              stroke="#3498DB"
              strokeWidth="2"
            />
            <circle
              cx="24"
              cy="24"
              r="18"
              fill="#34495E"
              stroke="#3498DB"
              strokeWidth="2"
            />
            <circle cx="24" cy="24" r="10" fill="#2980B9" />
            <path d="M24 6V42M6 24H42" stroke="#ECF0F1" strokeWidth="2" />
            <circle cx="24" cy="24" r="5" fill="#E74C3C" />
          </svg>
          <span className="text-2xl font-bold bg-clip-text text-transparent bg-gradient-to-r from-[#3498DB] to-[#E74C3C]">
            MemLens
          </span>
        </div>
        <nav>
          <ul className="flex space-x-6">
            <li>
              <a href="#" className="hover:text-[#3498DB]">
                Quick Start
              </a>
            </li>
            <li>
              <a href="#" className="hover:text-[#3498DB]">
                Documentation
              </a>
            </li>
            <li>
              <a href="#" className="hover:text-[#3498DB]">
                Tutorials
              </a>
            </li>
          </ul>
        </nav>
        <div className="flex space-x-4">
          <Button
            variant="outline"
            size="icon"
            className="bg-black border-[#3498DB] text-white hover:bg-[#3498DB] hover:text-black"
          >
            <DiscordIcon className="h-5 w-5" />
            <span className="sr-only">Join us on Discord</span>
          </Button>
          <Button
            variant="outline"
            size="icon"
            className="bg-black border-[#E74C3C] text-white hover:bg-[#E74C3C] hover:text-black"
          >
            <GithubIcon className="h-5 w-5" />
            <span className="sr-only">Star Us on GitHub</span>
          </Button>
          <Button
            variant="default"
            className="bg-gradient-to-r from-[#3498DB] to-[#E74C3C] text-black hover:from-[#E74C3C] hover:to-[#3498DB]"
          >
            Try It Now
          </Button>
        </div>
      </header>
      <main className="container mx-auto px-4 py-20 text-center relative z-10">
        <h1 className="text-7xl font-bold mb-6 text-white">
          VISUALIZE
          <br />
          <span className="bg-gradient-to-r from-[#3498DB] to-[#E74C3C] text-transparent bg-clip-text">
            MEMORY.
          </span>
        </h1>
        <h2 className="text-4xl font-bold mb-12 text-[#ECF0F1]">
          OPTIMIZE PERFORMANCE
        </h2>
        <Button
          size="lg"
          className="text-lg px-8 py-4 bg-gradient-to-r from-[#3498DB] to-[#E74C3C] text-black hover:from-[#E74C3C] hover:to-[#3498DB]"
        >
          Analyze Now
        </Button>
      </main>
      <div className="absolute inset-0 bg-black opacity-100 pointer-events-none">
        <svg width="100%" height="100%" xmlns="http://www.w3.org/2000/svg">
          <defs>
            {/* Change the flame gradient to darker tones */}
            <linearGradient
              id="flameGradient"
              x1="0%"
              y1="0%"
              x2="0%"
              y2="100%"
            >
              <stop offset="0%" stopColor="#333" />
              <stop offset="50%" stopColor="#444" />
              <stop offset="100%" stopColor="#555" />
            </linearGradient>
            <pattern
              id="hexPattern"
              x="0"
              y="0"
              width="100"
              height="100"
              patternUnits="userSpaceOnUse"
            >
              <text
                x="50"
                y="50"
                fontFamily="monospace"
                fontSize="10"
                fill="#888"  // Set text color to gray
                textAnchor="middle"
                dominantBaseline="middle"
              >
                {Math.floor(Math.random() * 256)
                  .toString(16)
                  .padStart(2, "0")}
              </text>
            </pattern>
          </defs>

          {/* Hexadecimal Background */}
          <rect width="100%" height="100%" fill="url(#hexPattern)" />

          {/* Realistic Flame Graph */}
          <g transform="translate(0, 500)">
            {Array.from({ length: 50 }).map((_, i) => (
              <rect
                key={i}
                x={`${i * 2}%`}
                y={-Math.random() * 200}
                width="2%"
                height={Math.random() * 200 + 50}
                fill="url(#flameGradient)"
                opacity="0.8"
              />
            ))}
          </g>

          {/* CPU Metrics Graph */}
          <g transform="translate(50, 100)">
            <path
              d="M0,100 Q50,-20 100,80 T200,60 T300,40 T400,70 T500,30"
              stroke="#888"  // Darker line color
              strokeWidth="3"
              fill="none"
            />
            <path
              d="M0,100 Q50,150 100,120 T200,140 T300,100 T400,130 T500,110"
              stroke="#444"  // Darker line color
              strokeWidth="3"
              fill="none"
            />
          </g>
        </svg>
      </div>
      {/* Floating icon */}
      <div className="absolute bottom-10 right-10">
        <BarChart2Icon className="h-16 w-16 text-[#888] animate-pulse" /> {/* Darker floating icon */}
      </div>
    </Main>
  );
}

