import { C } from "../constants";

export default function AnomalyDot({ cx, cy, payload, baseline, metricField, lowerBetter, onClick }) {
  if (!cx || !cy) return null;
  const bval = baseline?.[metricField]?.mean;
  let fill = C.accent;
  if (bval != null) {
    const worse = lowerBetter ? payload[metricField] > bval : payload[metricField] < bval;
    fill = worse ? C.red : C.green;
  }
  return <circle cx={cx} cy={cy} r={6} fill={fill} stroke={C.bg2} strokeWidth={2} style={{ cursor: "pointer" }} onClick={() => onClick && onClick(payload)} />;
}
