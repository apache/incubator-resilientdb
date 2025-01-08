import { useContext, useState } from "react";
import { Flamegraph } from "./flamegraph";
import { CpuLineGraphFunc } from "./lineGraph";
import { ModeType } from "../toggle";
import { ModeContext } from "@/hooks/context";
import { TourProvider } from "@/hooks/use-tour";

export function CpuPage() {
  const mode = useContext<ModeType>(ModeContext);
  const [date, setDate] = useState({
    from: 0,
    until: 0,
  });

  return (
    <TourProvider>
      {!(mode === "offline") && <CpuLineGraphFunc setDate={setDate} />}
      <Flamegraph {...date} />
    </TourProvider>
  );
}
