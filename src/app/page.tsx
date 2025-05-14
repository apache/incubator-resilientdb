"use client";

import { Button } from "@/components/ui/button";
import { useRouter } from "next/navigation";

export default function WelcomePage() {
  const router = useRouter();
  return (
    <div className="flex flex-col items-center justify-center min-h-[calc(100vh-10rem)] max-w-5xl mx-auto">
      <div className="text-center px-4">
        <h1 className="text-5xl font-bold mb-6 bg-gradient-to-r from-purple-500 to-indigo-600 text-transparent bg-clip-text">
          Explore Your Data with ResAI
        </h1>
        <p className="text-xl text-gray-300 mb-8 max-w-2xl mx-auto">
          Our modern interface with the new tubelight navigation makes it easy to interact with your documents and get intelligent insights.
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
            router.push('/placeholder');
          }}>
            View Placeholder
          </Button>
        </div>
      </div>

      {/* Feature Cards */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mt-16 w-full">
        <div className="bg-card/10 backdrop-blur-sm p-6 rounded-lg border border-border">
          <h3 className="text-xl font-semibold mb-3">Modern UI</h3>
          <p className="text-muted-foreground">Sleek navigation with animated transitions and responsive design.</p>
        </div>
        <div className="bg-card/10 backdrop-blur-sm p-6 rounded-lg border border-border">
          <h3 className="text-xl font-semibold mb-3">Intelligent Chat</h3>
          <p className="text-muted-foreground">Interact with your documents through natural language conversations.</p>
        </div>
        <div className="bg-card/10 backdrop-blur-sm p-6 rounded-lg border border-border">
          <h3 className="text-xl font-semibold mb-3">Document Analysis</h3>
          <p className="text-muted-foreground">Extract insights and information from your documents automatically.</p>
        </div>
      </div>
    </div>
  );
}
