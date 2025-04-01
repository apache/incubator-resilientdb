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
