"use client";

import { Button } from "@/components/ui/button";
import { LavaLamp } from "@/components/ui/fluid-blob";
import { useRouter } from "next/navigation";

export default function WelcomePage() {
  const router = useRouter();
  return (
    <div className="h-screen w-screen flex flex-col justify-center items-center relative">
        <LavaLamp />
        <h1 className="text-6xl md:text-8xl font-bold tracking-tight mix-blend-exclusion text-white whitespace-nowrap">
        Welcome to Nexus
        </h1>
        <p className="text-lg md:text-xl text-center text-white mix-blend-exclusion max-w-2xl leading-relaxed">
          Explore ResilientDB’s research, consensus protocols, and innovations. Instantly turn new ideas into working code.
        </p>
        <div className="flex flex-wrap justify-center gap-4 mt-8 z-10">
          <Button
            onClick={() => {
              router.push("/research");
            }}
          >
            Start Chatting
          </Button>
          <Button
            variant="secondary"
            onClick={() => {
              router.push("/graphql-tutor");
            }}
          >
            GraphQL Tutor
          </Button>
          <Button
            variant="secondary"
            onClick={() => {
              window.open("https://resilientdb.apache.org/", "_blank");
            }}
          >
            Official Website
          </Button>
        </div>
    </div>
  );
}
