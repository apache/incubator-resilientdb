import { C } from "../constants";

export default function Spinner() {
  return (
    <div style={{ display: "flex", alignItems: "center", justifyContent: "center", padding: "4rem", color: C.text3, fontSize: 12, gap: 10 }}>
      <div style={{ width: 16, height: 16, borderRadius: "50%", border: `2px solid ${C.border2}`, borderTopColor: C.accent, animation: "spin 0.8s linear infinite" }} />
      Loading from database...
      <style>{`@keyframes spin { to { transform: rotate(360deg); } }`}</style>
    </div>
  );
}
