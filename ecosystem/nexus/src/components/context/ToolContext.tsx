import { createContext, useContext, useState } from "react";

export type Tool = "default" | "code-composer";

const ToolContext = createContext<{
  activeTool: Tool;
  setTool: (t: Tool) => void;
} | null>(null);

export const ToolProvider: React.FC<{ children: React.ReactNode }> = ({ children }) => {
  const [activeTool, setTool] = useState<Tool>("default");
  return (
    <ToolContext.Provider value={{ activeTool, setTool }}>
      {children}
    </ToolContext.Provider>
  );
};

export const useTool = () => {
  const ctx = useContext(ToolContext);
  if (!ctx) throw new Error("useTool must be inside ToolProvider");
  return ctx;
}; 