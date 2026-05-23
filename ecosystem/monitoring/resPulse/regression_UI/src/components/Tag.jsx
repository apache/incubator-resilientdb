export default function Tag({ children, color, bg, border }) {
  return (
    <span style={{ display: "inline-flex", alignItems: "center", gap: 5, padding: "3px 10px", borderRadius: 4, fontSize: 10, letterSpacing: 1, fontWeight: 700, color, background: bg, border: `1px solid ${border}` }}>
      {children}
    </span>
  );
}
