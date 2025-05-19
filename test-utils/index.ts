import userEvent from '@testing-library/user-event';

export * from '@testing-library/react';
export { render } from './render';
export { userEvent };
export async function loadPyodideInstance() {
    if (!window.loadPyodide) {
      const pyodideScript = document.createElement("script");
      pyodideScript.src = "https://cdn.jsdelivr.net/pyodide/v0.23.4/full/pyodide.js";
      document.head.appendChild(pyodideScript);
      await new Promise((resolve) => (pyodideScript.onload = resolve));
    }
  
    return await window.loadPyodide({ indexURL: "https://cdn.jsdelivr.net/pyodide/v0.23.4/full/" });
  }
  
