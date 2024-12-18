import { useContext, useState } from "react";
import { Flamegraph } from "./flamegraph";
import { CpuLineGraphFunc } from "./lineGraph";
import { ModeType } from "../toggle";
import { ModeContext } from "@/hooks/context";

export function CpuPage() {
  const mode = useContext<ModeType>(ModeContext);
  const [date, setDate] = useState({
    from: 0,
    until: 0,
  });

  return (
    <>
      {!(mode === "offline") && <CpuLineGraphFunc setDate={setDate} />}
      <Flamegraph {...date} />
    </>
  );
}
