import { clsx, type ClassValue } from "clsx";
import { twMerge } from "tailwind-merge";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

export function cleanUpImplementation(implementation: string) {
  implementation = implementation.replace(/__CODE_COMPOSER_META__[\s\S]*$/, '').trim();
  
  // remove closing explanation after code blocks (starts after final ``` and contains explanatory text)
  const lastCodeBlockEnd = implementation.lastIndexOf('```');
  if (lastCodeBlockEnd !== -1) {
    const afterCodeBlock = implementation.substring(lastCodeBlockEnd + 3).trim();
    // if there's substantial explanatory text after the code block, remove it
    if (afterCodeBlock.length > 50 && afterCodeBlock.includes('This implementation')) {
      implementation = implementation.substring(0, lastCodeBlockEnd + 3).trim();
    }
  }
  // remove any trailing markdown code block markers
  implementation = implementation.replace(/```\s*$/, '').trim();

  return implementation;
} 