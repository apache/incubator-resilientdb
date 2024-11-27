import { ClassValue, clsx } from "clsx";
import { twMerge } from "tailwind-merge";

export function cn(...inputs: ClassValue[]) {
  return twMerge(clsx(inputs));
}

export function startCase(str: string) {
  return str
    .replace(/_/g, ' ')        // Replace underscores with spaces
    .replace(/-/g, ' ')        // Replace dashes with spaces
    .replace(/\s+/g, ' ')      // Normalize multiple spaces
    .trim()                    // Trim leading/trailing spaces
    .toLowerCase()             // Convert to lowercase
    .replace(/\b\w/g, char => char.toUpperCase()); // Capitalize the first letter of each word
}
