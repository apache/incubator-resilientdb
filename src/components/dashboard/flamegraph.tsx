"use client";

import "@pyroscope/flamegraph/dist/index.css";

import { FlamegraphRenderer, Box } from "@pyroscope/flamegraph";
import { ProfileData1 } from "@/mock/test_flamegraph";

export const Flamegraph = () => {
  return (
    <Box>
      <FlamegraphRenderer
        profile={ProfileData1}
        showCredit={false}
        onlyDisplay="both"
        showToolbar={true}
      />
    </Box>
  );
};
