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
              window.open("https://resilientdb.apache.org/", "_blank");
            }}
          >
            Official Website
          </Button>
        </div>
      {/* Feature Cards */}
      {/* <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mt-16 w-full">
        <div className="bg-card/10 backdrop-blur-sm p-6 rounded-lg border border-border">
          <h3 className="text-xl font-semibold mb-3">Development SDK</h3>
          <p className="text-muted-foreground">Access Python, Rust and TypeScript SDKs to build applications on ResilientDB's blockchain fabric.</p>
        </div>
        <div className="bg-card/10 backdrop-blur-sm p-6 rounded-lg border border-border">
          <h3 className="text-xl font-semibold mb-3">Smart Contracts</h3>
          <p className="text-muted-foreground">Implement Solidity-based smart contracts with ResContract CLI for deployment and execution.</p>
        </div>
        <div className="bg-card/10 backdrop-blur-sm p-6 rounded-lg border border-border">
          <h3 className="text-xl font-semibold mb-3">Key-Value Store</h3>
          <p className="text-muted-foreground">Utilize version-based and non-version-based key-value storage for blockchain applications.</p>
        </div>
     
      </div> */}
    </div>
  );
}
