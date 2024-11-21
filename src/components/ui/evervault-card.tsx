import { useMotionValue, useMotionTemplate, motion } from "framer-motion";
import { useState, useEffect } from "react";
import { cn } from "@/lib/utils";

export const EvervaultCard = ({ className }: { className?: string }) => {
  const mouseX = useMotionValue(0);
  const mouseY = useMotionValue(0);
  const [randomString, setRandomString] = useState("");
  const [text, setText] = useState("");

  useEffect(() => {
    // Generate the initial random background string
    setRandomString(generateRandomString(10000));
    // Create the typewriter effect
    typeWriterEffect(
      "MemLens :\nThe ResiliantDB Memory Profiler",
      setText,
      100
    );
  }, []);

  function onMouseMove({ currentTarget, clientX, clientY }: any) {
    const { left, top } = currentTarget.getBoundingClientRect();
    mouseX.set(clientX - left);
    mouseY.set(clientY - top);
  }

  return (
    <div
      className={cn(
        "w-full h-screen relative flex items-center justify-center overflow-hidden",
        className
      )}
    >
      <div
        onMouseMove={onMouseMove}
        className="group/card w-full relative overflow-hidden flex items-center justify-center h-full"
      >
        <CardPattern mouseX={mouseX} mouseY={mouseY} randomString={randomString} />
        <div className="relative z-10 text-center">
          <span
            className="dark:text-white text-black text-5xl font-bold"
            style={{ WebkitTextStroke: "1px black" }}
          >
            {text}
          </span>
        </div>
      </div>
    </div>
  );
};

// Generates the moving spotlight effect and the random string background
function CardPattern({ mouseX, mouseY, randomString }: any) {
  const maskImage = useMotionTemplate`radial-gradient(250px at ${mouseX}px ${mouseY}px, white, transparent)`;
  const style = { maskImage, WebkitMaskImage: maskImage };

  return (
    <div className="pointer-events-none">
      {/* Translucent Background */}
      <div className="absolute inset-0 bg-gradient-to-r from-green-500 to-blue-700 opacity-40"></div>

      {/* Spotlight with string */}
      <motion.div
        className="absolute inset-0 opacity-100 mix-blend-overlay group-hover/card:opacity-100"
        style={style}
      >
        <p className="absolute inset-x-0 text-xs h-full break-words whitespace-pre-wrap text-white font-mono font-bold transition duration-500">
          {randomString}
        </p>
      </motion.div>
    </div>
  );
}

// Function for generating random background strings
const characters =
  "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
export const generateRandomString = (length: number) => {
  let result = "";
  for (let i = 0; i < length; i++) {
    result += characters.charAt(Math.floor(Math.random() * characters.length));
  }
  return result;
};

// Custom Typewriter Effect
const typeWriterEffect = (
  fullText: string,
  setText: (text: string) => void,
  speed: number
) => {
  let index = 0;
  let currentText = ""; // Local variable to track progress

  const typing = () => {
    if (index < fullText.length) {
      currentText += fullText[index]; // Add the next character
      setText(currentText); // Update the state with the new value
      index++;
      setTimeout(typing, speed); // Recursive call with delay
    }
  };

  typing();
};
