import * as React from "react";
import { Check, ChevronDown, Globe, Wifi, WifiOff } from "lucide-react";

import { cn } from "@/lib/utils";
import { Button } from "@/components/ui/button";
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuItem,
  DropdownMenuTrigger,
} from "@/components/ui/dropdown-menu";

export type ModeType = "offline" | "live" | "remote";

const states: {
  value: ModeType;
  label: string;
  disabled: boolean;
  icon: React.ComponentType<{ className?: string }>;
}[] = [
  {
    value: "offline",
    label: "Offline",
    icon: WifiOff,
    disabled: false,
  },
  {
    value: "live",
    label: "Live",
    icon: Wifi,
    disabled: false,
  },
  {
    value: "remote",
    label: "Remote",
    icon: Globe,
    disabled: true,
  },
];

export function ToggleState({ state, setState }) {
  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <Button variant="outline" className="w-[120px] justify-between">
          <div className="flex items-center">
            {React.createElement(
              states.find((s) => s.value === state)
                ?.icon as React.ComponentType<{ className?: string }>,
              {
                className: "mr-2 h-4 w-4",
              }
            )}
            {states.find((s) => s.value === state)?.label}
          </div>
          <ChevronDown className="ml-auto h-4 w-4 shrink-0 opacity-50" />
        </Button>
      </DropdownMenuTrigger>
      <DropdownMenuContent className="w-[120px]">
        {states.map((stateOption) => (
          <DropdownMenuItem
            disabled={stateOption.disabled}
            key={stateOption.value}
            onClick={() => setState(stateOption.value)}
          >
            <stateOption.icon className="mr-2 h-4 w-4" />
            <span>{stateOption.label}</span>
            <Check
              className={cn(
                "ml-auto h-4 w-4",
                stateOption.value === state ? "opacity-100" : "opacity-0"
              )}
            />
          </DropdownMenuItem>
        ))}
      </DropdownMenuContent>
    </DropdownMenu>
  );
}
