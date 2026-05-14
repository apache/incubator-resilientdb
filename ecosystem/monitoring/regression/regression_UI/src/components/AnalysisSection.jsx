import { C } from "../constants";

export function Em({ children }) {
  return <strong style={{ color: C.text, fontWeight: 600 }}>{children}</strong>;
}

export default function AnalysisSection({ label, children }) {
  return (
    <div style={{ background: C.bg3, border: `1px solid ${C.border2}`, borderRadius: 8, padding: "1.2rem 1.4rem", marginBottom: "1.2rem", position: "relative" }}>
      <div style={{ position: "absolute", top: -8, left: 12, fontSize: 9, letterSpacing: 2, color: C.accent, background: C.bg3, padding: "0 6px", fontWeight: 700 }}>{label}</div>
      {children}
    </div>
  );
}
