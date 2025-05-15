"use client";

import { Button } from "@/components/ui/button";
import { useRouter } from "next/navigation";

export default function WelcomePage() {
  const router = useRouter();
  return (
    <div className="flex flex-col justify-center items-center flex-1 h-screen max-w-5xl mx-auto">
      <div className="text-center px-4">
        <h1 className="text-5xl font-bold mb-6 bg-gradient-to-r from-purple-500 to-indigo-600 text-transparent bg-clip-text">
          ResilientDB's AI-Powered Resource Hub
        </h1>
        <p className="text-xl text-gray-300 mb-8 max-w-2xl mx-auto">
          Access everything for Apache ResilientDB (Incubating) — the high-throughput distributed ledger built on scale-centric design principles.
        </p>
        <div className="flex flex-wrap justify-center gap-4 mt-8">
          <Button
          onClick={() => {
            router.push('/chat');
          }}
          >
            Start Chatting
          </Button>
          <Button variant="secondary" onClick={() => {
            window.open('https://resilientdb.apache.org/', '_blank');
          }}>
            Official Website
          </Button>
        </div>
      </div>

      {/* Feature Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mt-16 w-full">
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
     
      </div>
    </div>
  );
}
