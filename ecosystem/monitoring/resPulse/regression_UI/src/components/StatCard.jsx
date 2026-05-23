import { C } from "../constants";

export default function StatCard({ label, value, delta, anomaly }) {
  return (
    <div style={{ background: C.bg2, border: `1px solid ${anomaly ? "rgba(255,71,87,0.35)" : C.border}`, borderRadius: 8, padding: "1rem 1.2rem", position: "relative", overflow: "hidden" }}>
      <div style={{ position: "absolute", top: 0, left: 0, right: 0, height: 2, background: anomaly ? C.red : C.accent, opacity: anomaly ? 1 : 0.4 }} />
      <div style={{ fontSize: 10, color: C.text3, letterSpacing: 2, textTransform: "uppercase", marginBottom: 6 }}>{label}</div>
      <div style={{ fontSize: 22, fontWeight: 700, lineHeight: 1, marginBottom: 4 }}>{value}</div>
      <div style={{ fontSize: 11 }}>{delta}</div>
    </div>
  );
}
