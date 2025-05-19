"use client"
import { useEffect, useRef, useState } from "react"
import { loadPyodideInstance } from "@/components/utils/loadPyodide"

export default function PythonSplitIDE({ defaultCode }: { defaultCode: string }) {
  const [pyodide, setPyodide] = useState<any>(null)
  const [output, setOutput] = useState<string[]>(["Initializing Python..."])
  const [isLoading, setIsLoading] = useState(true)
  const [isRunning, setIsRunning] = useState(false)
  const textareaRef = useRef<HTMLTextAreaElement>(null)

  useEffect(() => {
    async function init() {
      try {
        const py = await loadPyodideInstance()

        py.globals.set("js_print", (text: string) => {
          setOutput(prev => [...prev, text])
        })

        await py.runPythonAsync(`import builtins\nbuiltins.print = js_print`)

        const sdk = await fetch("/resdb_sdk.py").then(res => res.text())
        await py.runPythonAsync(sdk)

        setPyodide(py)
        setOutput(["✅ Python environment loaded."])
        setIsLoading(false)
      } catch (e) {
        console.error(e)
        setOutput(["❌ Failed to initialize Python."])
        setIsLoading(false)
      }
    }

    init()
  }, [])

  const runCode = async () => {
    if (!pyodide || !textareaRef.current) return
    setIsRunning(true)
    setOutput(prev => [...prev, "\n▶ Running code..."])

    try {
      const code = textareaRef.current.value
      await pyodide.runPythonAsync(code)
      setOutput(prev => [...prev, "✅ Code execution completed."])
    } catch (err: any) {
      setOutput(prev => [...prev, `❌ Error: ${err.message}`])
    } finally {
      setIsRunning(false)
    }
  }

  return (
    <div className="flex flex-col md:flex-row gap-4 w-full my-6">
      {/* Left side: Editor */}
      <div className="flex-1 bg-black rounded-lg p-4 border border-slate-700">
        <textarea
          ref={textareaRef}
          defaultValue={defaultCode}
          className="w-full h-[400px] bg-black text-green-200 font-mono text-sm p-4 rounded resize-none border border-slate-600"
          placeholder="Write your Python code here..."
          disabled={isLoading}
        />
        <div className="mt-4 flex gap-2">
          <button
            onClick={runCode}
            disabled={isLoading || isRunning}
            className="px-4 py-2 rounded bg-blue-600 text-white text-sm hover:bg-blue-700"
          >
            {isRunning ? "Running..." : "Run Code"}
          </button>
          <button
            onClick={() => setOutput([])}
            className="px-4 py-2 rounded bg-slate-700 text-white text-sm hover:bg-slate-600"
          >
            Clear Output
          </button>
        </div>
      </div>

      {/* Right side: Output */}
      <div className="flex-1 bg-slate-900 text-green-400 font-mono text-sm p-4 border border-slate-700 rounded-lg overflow-auto h-[400px]">
        {output.length === 0 ? (
          <div className="text-slate-500">No output yet.</div>
        ) : (
          output.map((line, i) => <div key={i}>{line}</div>)
        )}
        {isLoading && <div className="text-slate-400">⏳ Loading Python...</div>}
        {isRunning && <div className="text-slate-400">⚙️ Running...</div>}
      </div>
    </div>
  )
}
