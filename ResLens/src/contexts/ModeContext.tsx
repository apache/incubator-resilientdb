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

import { createContext, useContext, useState, useEffect, ReactNode, useMemo } from "react";
import { middlewareApi, middlewareSecondaryApi } from "@/lib/api";
import { ModeType } from "@/components/toggle";

interface ModeContextType {
  mode: ModeType;
  isLoading: boolean;
  setMode: (mode: ModeType) => void;
  checkMode: (instance?: string) => Promise<void>;
  currentInstance: string;
  error: string | null;
  api: typeof middlewareApi; // Add the API client
  refreshTrigger: number; // Add a refresh trigger
}

const ModeContext = createContext<ModeContextType | undefined>(undefined);

interface ModeProviderProps {
  children: ReactNode;
}

export function ModeProvider({ children }: ModeProviderProps) {
  const [mode, setMode] = useState<ModeType>("prod");
  const [isLoading, setIsLoading] = useState(true);
  const [currentInstance, setCurrentInstance] = useState<string>("production");
  const [error, setError] = useState<string | null>(null);
  const [refreshTrigger, setRefreshTrigger] = useState(0);

  async function checkMode(instance?: string) {
    try {
      setIsLoading(true);
      setError(null); // Clear previous errors
      
      const targetInstance = instance || "production";
      
      // Validate instance parameter
      const validInstances = ['development', 'production'];
      if (!validInstances.includes(targetInstance)) {
        const errorMsg = `Invalid instance: ${targetInstance}. Only 'development' and 'production' are allowed.`;
        console.error(errorMsg);
        setError(errorMsg);
        setMode("prod");
        setIsLoading(false);
        return;
      }
      
      // Skip if we're already using the correct instance
      if (currentInstance === targetInstance && mode === "prod") {
        console.log(`Already using ${targetInstance} API, skipping health check`);
        return;
      }
      
      // Choose API based on instance
      const api = targetInstance === 'development' ? middlewareSecondaryApi : middlewareApi;
      
      console.log(`Using ${targetInstance} API for health check`);
      
      const response = await api.get("/healthcheck");
      if (response?.status === 200) {
        setMode("prod");
        setCurrentInstance(targetInstance);
      } else {
        setMode("development");
      }
    } catch (error) {
      setMode("development");
    } finally {
      setIsLoading(false);
    }
  }

  // Function to handle mode changes and trigger appropriate API calls
  const handleModeChange = async (newMode: ModeType) => {
    try {
      setIsLoading(true);
      setError(null);
      
      // Map mode to instance
      const instance = newMode === "development" ? "development" : "production";
      
      // Choose API based on new mode
      const api = newMode === "development" ? middlewareSecondaryApi : middlewareApi;
      
      console.log(`Switching to ${newMode} mode, using ${instance} API`);
      
      // Always update the mode and trigger refresh immediately
      setMode(newMode);
      setCurrentInstance(instance);
      setRefreshTrigger(prev => {
        console.log(`Incrementing refreshTrigger from ${prev} to ${prev + 1} for mode change to ${newMode}`);
        return prev + 1;
      }); // Always trigger refresh
      
      // Try health check but don't block the mode change
      try {
        const response = await api.get("/healthcheck");
        if (response?.status === 200) {
          console.log(`Successfully connected to ${newMode} endpoint`);
        } else {
          console.warn(`Health check failed for ${newMode} endpoint, but mode changed`);
        }
      } catch (error) {
        console.warn(`Health check failed for ${newMode} endpoint: ${error.message}, but mode changed`);
      }
      
    } catch (error) {
      console.error(`Error during mode change to ${newMode}:`, error);
      setError(`Failed to connect to ${newMode} endpoint: ${error.message}`);
    } finally {
      setIsLoading(false);
    }
  };

  async function getMode() {
    await checkMode(); // Default to production
  }

  useEffect(() => {
    getMode();
  }, []);

  // Memoize the API selection to prevent infinite loops
  const selectedApi = useMemo(() => {
    return mode === "development" ? middlewareSecondaryApi : middlewareApi;
  }, [mode]);

  return (
    <ModeContext.Provider value={{ 
      mode, 
      isLoading, 
      setMode: handleModeChange, 
      checkMode, 
      currentInstance, 
      error,
      api: selectedApi,
      refreshTrigger
    }}>
      {children}
    </ModeContext.Provider>
  );
}

export function useMode() {
  const context = useContext(ModeContext);
  if (context === undefined) {
    throw new Error("useMode must be used within a ModeProvider");
  }
  return context;
} 