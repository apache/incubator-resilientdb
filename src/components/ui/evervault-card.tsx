import { useMotionValue, useMotionTemplate, motion } from "framer-motion";
import { useState, useEffect } from "react";
import { cn } from "@/lib/utils";

export const EvervaultCard = ({ className }: { className?: string }) => {
  const mouseX = useMotionValue(0); // Track mouse X position
  const mouseY = useMotionValue(0); // Track mouse Y position
  const [randomString, setRandomString] = useState("");
  const [firstLineText, setFirstLineText] = useState(""); // State for the first line
  const [secondLineText, setSecondLineText] = useState(""); // State for the second line

  useEffect(() => {
    // Generate the initial random background string
    setRandomString(generateRandomString(10100));

    // Update random string every 500 ms
    const interval = setInterval(() => {
      setRandomString(generateRandomString(10100));
    }, 500);

    // Create the typewriter effect with line breaks
    typeWriterEffect(
      "MemLens :", // First line to type
      setFirstLineText, // Function to update the first line text
      100, // Speed (controls how fast each character is typed)
      () => {
        // Callback to start the second line after the first one finishes
        typeWriterEffect(
          "The ResiliantDB Memory Profiler", // Second line to type
          setSecondLineText,
          70,
          () => {} // No callback needed after the second line finishes
        );
      }
    );

    // Cleanup interval on component unmount
    return () => clearInterval(interval);
  }, []);

  // Mouse move handler
  function onMouseMove({ currentTarget, clientX, clientY }: any) {
    const { left, top } = currentTarget.getBoundingClientRect();
    mouseX.set(clientX - left); // Set X relative to the card
    mouseY.set(clientY - top); // Set Y relative to the card
  }

  return (
    <div
      className={cn(
        "w-full h-screen relative flex items-center justify-center overflow-hidden",
        className
      )}
    >
      <div
        onMouseMove={onMouseMove} // Attach mouse tracking here
        className="group/card w-full relative overflow-hidden flex items-center justify-center h-full"
      >
        {/* Add the CardPattern with spotlight */}
        <CardPattern randomString={randomString} mouseX={mouseX} mouseY={mouseY} />
        <div className="relative z-10 text-center">
          <span
            className="dark:text-white text-black text-7xl font-bold"
            style={{ WebkitTextStroke: "3px black" }}
          >
            {firstLineText}
          </span>
          <br />
          <span
            className="dark:text-white text-black text-5xl font-medium"
            style={{ WebkitTextStroke: "1px black" }}
          >
            {secondLineText}
          </span>
        </div>
      </div>
    </div>
  );
};

// Generates the moving spotlight effect and the random string background
function CardPattern({ randomString, mouseX, mouseY }: any) {
  const maskImage = useMotionTemplate`radial-gradient(250px at ${mouseX}px ${mouseY}px, rgba(255,255,255,0.8), transparent)`; // Spotlight effect
  const style = { maskImage, WebkitMaskImage: maskImage };

  return (
    <div className="pointer-events-none">
      {/* Translucent Black Gradient Background */}
      <div className="absolute inset-0 bg-gradient-to-r from-black to-black opacity-40 z-0"></div>

      {/* Random string background */}
      <div className="absolute inset-0 opacity-30 mix-blend-lighten z-10">
        <p className="absolute inset-x-0 text-xs h-full break-words whitespace-pre-wrap text-green-500 font-mono font-bold transition duration-500 z-20">
          {randomString}
        </p>
      </div>

      {/* Spotlight */}
      <motion.div
        className="absolute inset-0 z-30"
        style={{
          ...style, // Apply spotlight style
          mixBlendMode: "normal", // Blend mode for spotlight
          opacity: 0.6, // Adjust spotlight visibility
        }}
      />
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

// Custom Typewriter Effect with line breaks support
const typeWriterEffect = (
  fullText: string,
  setText: (text: string) => void,
  speed: number,
  callback: () => void // Callback function to start typing second line
) => {
  let index = 0;
  let currentText = ""; // Local variable to track progress

  const typing = () => {
    if (index < fullText.length) {
      currentText += fullText[index]; // Add the next character
      setText(currentText); // Update the state with the new value
      index++;
      setTimeout(typing, speed); // Recursive call with delay
    } else {
      callback(); // Once typing is done, call the callback to start the second line
    }
  };

  typing();
};
