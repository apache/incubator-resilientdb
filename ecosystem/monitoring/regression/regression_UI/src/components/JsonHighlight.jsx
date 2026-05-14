import { C } from "../constants";

export default function JsonHighlight({ json }) {
  const highlighted = json.replace(
    /("(\\u[a-zA-Z0-9]{4}|\\[^u]|[^\\"])*"(\s*:)?|\b(true|false|null)\b|-?\d+(?:\.\d*)?(?:[eE][+-]?\d+)?)/g,
    (match) => {
      let color = C.purple;
      if (/^"/.test(match)) color = /:$/.test(match) ? C.accent : C.green;
      return `<span style="color:${color}">${match}</span>`;
    }
  );
  return (
    <pre style={{ fontSize: 11, lineHeight: 1.6, color: C.text2, overflow: "auto", whiteSpace: "pre" }} dangerouslySetInnerHTML={{ __html: highlighted }} />
  );
}
