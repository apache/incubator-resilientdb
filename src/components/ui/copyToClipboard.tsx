import React, { useState } from "react";
import { Check, Copy } from "lucide-react";
import { Button } from "@/components/ui/button";
import { Card, CardContent } from "@/components/ui/card";

interface CopyableCommandProps {
  command: string;
}

export const CopyableCommand: React.FC<CopyableCommandProps> = ({
  command,
}) => {
  const [isCopied, setIsCopied] = useState(false);

  const copyToClipboard = async () => {
    try {
      await navigator.clipboard.writeText(command);
      setIsCopied(true);
      setTimeout(() => setIsCopied(false), 2000);
    } catch (err) {
      console.error("Failed to copy text: ", err);
    }
  };

  return (
    <div className="w-full max-w-3xl mx-auto">
      <div className="p-4">
        <div className="flex bg-slate-800 justify-left rounded-md">
          <pre className="p-4 text-sm text-secondary-foreground overflow-x-auto">
            <code>{command}</code>
          </pre>
          <Button
            variant="ghost"
            size="icon"
            className="mt-2 mr-2"
            onClick={copyToClipboard}
            aria-label={isCopied ? "Copied" : "Copy to clipboard"}
          >
            {isCopied ? (
              <Check className="h-4 w-4 text-green-500" />
            ) : (
              <Copy className="h-4 w-4" />
            )}
          </Button>
        </div>
      </div>
    </div>
  );
};
