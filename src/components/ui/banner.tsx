import { useState } from "react";
import { Info, X } from "lucide-react";
import { Alert } from "./alert";

export default function Banner() {
  const [isVisible, setIsVisible] = useState(true);

  if (!isVisible) return null;

  return (
    <Alert className="bg-blue-600  text-white px-4 py-3 flex items-center justify-between">
      <div className="flex items-center">
        <Info className="h-5 w-5 mr-2" />
        <span>
          <strong>You're exploring offline mode!</strong>
          <br />
          Enjoy the static data for now. Stay tuned—live data and interactive
          features are coming your way soon!✨
        </span>
      </div>
      <button
        onClick={() => setIsVisible(false)}
        className="text-white hover:text-blue-100 transition-colors"
        aria-label="Close banner"
      >
        <X className="h-5 w-5" />
      </button>
    </Alert>
  );
}
