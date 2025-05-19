import { forwardRef } from "react"
import { cn } from "@/app/lib/utils"

interface TerminalProps extends React.HTMLAttributes<HTMLDivElement> {
  children: React.ReactNode
}

export const Terminal = forwardRef<HTMLDivElement, TerminalProps>(
  ({ children, className, ...props }, ref) => {
    return (
      <div
        ref={ref}
        className={cn(
          "bg-black border border-slate-700 rounded-b-lg p-4 font-mono text-sm text-green-400 overflow-auto max-h-[400px]",
          className,
        )}
        {...props}
      >
        {children}
      </div>
    )
  }
)

Terminal.displayName = "Terminal"