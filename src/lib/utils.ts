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

export function formatSeconds(createdAt: string) {
  const now = Date.now(); // Current time in milliseconds
  const createdAtDate = new Date(createdAt);
  const diffInSeconds = Math.floor((now - createdAtDate.getTime()) / 1000);
  const days = Math.floor(diffInSeconds / 86400);
  const hours = Math.floor((diffInSeconds % 86400) / 3600);
  const minutes = Math.floor((diffInSeconds % 3600) / 60);
  const seconds = diffInSeconds % 60;

  let timeString = "";
  if (days > 0) timeString += `${days}d `;
  if (hours > 0) timeString += `${hours}h `;
  if (minutes > 0) timeString += `${minutes}m `;
  if (seconds > 0) timeString += `${seconds}s`;

  return timeString.trim();
}