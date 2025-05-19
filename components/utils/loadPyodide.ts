declare global {
  interface Window {
    loadPyodide: (options: { indexURL: string }) => Promise<any>
  }
}

let pyodidePromise: Promise<any> | null = null

export async function loadPyodideInstance() {
  if (pyodidePromise) return pyodidePromise

  pyodidePromise = new Promise(async (resolve, reject) => {
    try {
      if (!window.loadPyodide) {
        const script = document.createElement("script")
        script.src = "https://cdn.jsdelivr.net/pyodide/v0.23.4/full/pyodide.js"
        script.onerror = () => reject(new Error("❌ Failed to load Pyodide script"))
        document.head.appendChild(script)

        await new Promise<void>((resolveScript) => {
          script.onload = () => resolveScript()
        })
      }

      const pyodide = await window.loadPyodide({
        indexURL: "https://cdn.jsdelivr.net/pyodide/v0.23.4/full/",
      })

      resolve(pyodide)
    } catch (err) {
      pyodidePromise = null
      reject(err)
    }
  })

  return pyodidePromise
}
