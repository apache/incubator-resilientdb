import React, {
  createContext,
  useContext,
  useState,
  useCallback,
  useEffect,
} from "react";
import { Step } from "react-joyride";

interface TourContextType {
  isRunning: boolean;
  startTour: () => void;
  endTour: () => void;
  steps: Step[];
  setSteps: (steps: Step[]) => void;
  autoStart: boolean;
  setAutoStart: (autoStart: boolean) => void;
}

const TourContext = createContext<TourContextType | undefined>(undefined);

export const TourProvider: React.FC<{
  children: React.ReactNode;
  autoStart?: boolean;
}> = ({ children, autoStart = false }) => {
  const [isRunning, setIsRunning] = useState(false);
  const [steps, setSteps] = useState<Step[]>([]);
  const [shouldAutoStart, setAutoStart] = useState(autoStart);

  const startTour = useCallback(() => {
    console.log("startTour");
    setIsRunning(true);
  }, []);
  const endTour = useCallback(() => setIsRunning(false), []);

  useEffect(() => {
    if (shouldAutoStart && steps.length > 0) {
      startTour();
    }
  }, [shouldAutoStart, steps, startTour]);

  return (
    <TourContext.Provider
      value={{
        isRunning,
        startTour,
        endTour,
        steps,
        setSteps,
        autoStart: shouldAutoStart,
        setAutoStart,
      }}
    >
      {children}
    </TourContext.Provider>
  );
};

export const useTour = () => {
  const context = useContext(TourContext);
  if (context === undefined) {
    throw new Error("useTour must be used within a TourProvider");
  }
  return context;
};
