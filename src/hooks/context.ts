import { ModeType } from "@/components/toggle";
import { createContext } from "react";

export const ModeContext = createContext<ModeType>("live");