import { useState } from "react";
import { Flamegraph } from "./flamegraph";
import { CpuLineGraphFunc } from "./lineGraph";

export function CpuPage() {
  const [date, setDate] = useState({
    from: 0,
    until: 0,
  });

  return (
    <>
      <CpuLineGraphFunc setDate={setDate} />
      <Flamegraph {...date} />
    </>
  );
}
