import React from "react";
import { EvervaultCard } from "../ui/evervault-card";

export function EvervaultCardDemo() {
  return (
    <div className="relative w-full h-screen">
      <EvervaultCard className="absolute top-0 left-0 w-full h-full" />
    </div>
  );
}
