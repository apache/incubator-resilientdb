//@ts-nocheck
import React from "react";
import { cn } from "@/lib/utils";
import { Sparkles } from "lucide-react";
import Joyride, { CallBackProps, STATUS } from "react-joyride";
import { useTour } from "@/hooks/use-tour";

export function Tour() {
  const { isRunning, endTour, steps } = useTour();

  const handleJoyrideCallback = (data: CallBackProps) => {
    const { status } = data;
    const finishedStatuses: string[] = [STATUS.FINISHED, STATUS.SKIPPED];

    if (finishedStatuses.includes(status)) {
      endTour();
    }
  };

  return (
    <Joyride
      callback={handleJoyrideCallback}
      continuous
      run={isRunning}
      scrollOffset={64}
      showProgress
      showSkipButton
      spotlightClicks={false}
      steps={steps}
      beaconComponent={CustomBeacon}
      locale={{
        last: "Finish",
        skip: "Skip",
      }}
      styles={{
        options: {
          arrowColor: "hsl(var(--card))",
          backgroundColor: "rgb(2 6 23 / 0.8)",
          primaryColor: "#3b82f6",
          textColor: "hsl(var(--card-foreground))",
          zIndex: 1000,
        },
      }}
    />
  );
}

interface CustomBeaconProps {
  onClick: () => void;
}

export const CustomBeacon: React.FC<CustomBeaconProps> = ({ onClick }) => {
  return (
    <div
      className={cn(
        "rounded-lg",
        "relative inline-flex items-center justify-center",
        "cursor-pointer",
        "transition-all duration-300 ease-in-out",
        "hover:scale-110"
      )}
      onClick={onClick}
    >
      <span
        className={cn(
          "rounded-lg",
          "animate-ping absolute inline-flex h-full w-full rounded-xl",
          "bg-blue-500"
        )}
      ></span>
      <span
        className={cn(
          "relative inline-flex rounded-xl h-6 w-6",
          "bg-blue-500 text-white",
          "items-center justify-center"
        )}
      >
        <Sparkles size={16} />
      </span>
    </div>
  );
};
