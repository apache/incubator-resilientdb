"use client";
import React, { useState } from "react";
import { motion } from "framer-motion";
import { useNavigate } from "react-router-dom";
import { cn } from "@/lib/utils";

export const WobbleCard = ({
  children,
  containerClassName,
  className,
}: {
  children: React.ReactNode;
  containerClassName?: string;
  className?: string;
}) => {
  const [mousePosition, setMousePosition] = useState({ x: 0, y: 0 });
  const [isHovering, setIsHovering] = useState(false);

  const navigate = useNavigate();

  const handleMouseMove = (event: React.MouseEvent<HTMLElement>) => {
    const { clientX, clientY } = event;
    const rect = event.currentTarget.getBoundingClientRect();
    const x = (clientX - (rect.left + rect.width / 2)) / 60;
    const y = (clientY - (rect.top + rect.height / 2)) / 60;
    setMousePosition({ x, y });
  };

  const handleNavigateDashboard = () => {
    console.log("Navigating to /dashboard");
    navigate("/dashboard");
  };

  const handleOtherAction = () => {
    console.log("Second button clicked");
    // Placeholder for other action
  };

  return (
    <motion.section
      onMouseMove={handleMouseMove}
      onMouseEnter={() => setIsHovering(true)}
      onMouseLeave={() => {
        setIsHovering(false);
        setMousePosition({ x: 0, y: 0 });
      }}
      style={{
        transform: isHovering
          ? `translate3d(${mousePosition.x}px, ${mousePosition.y}px, 0) scale3d(1.01, 1.01, 1) rotate3d(0.8, 0.8, 0, 3deg)`
          : "translate3d(0px, 0px, 0) scale3d(1, 1, 1) rotate3d(0, 0, 0, 0deg)",
        transition: "transform 0.2s ease-out",
      }}
      className={cn(
        "mx-auto w-full bg-black relative rounded-2xl overflow-hidden shadow-xl",
        containerClassName
      )}
    >
      <div
        className="relative h-full sm:mx-0 sm:rounded-2xl overflow-hidden"
        style={{
          boxShadow:
            "0 10px 32px rgba(34, 42, 53, 0.2), 0 1px 1px rgba(0, 0, 0, 0.1), 0 0 0 1px rgba(34, 42, 53, 0.1), 0 4px 6px rgba(34, 42, 53, 0.15), 0 24px 108px rgba(47, 48, 55, 0.15)",
        }}
      >
        <motion.div
          style={{
            transform: isHovering ? "scale3d(1.05, 1.05, 1)" : "scale3d(1, 1, 1)",
            transition: "transform 0.2s ease-out",
          }}
          className={cn(
            "h-full px-6 py-16 sm:px-10 text-gray-200 font-sans tracking-wide",
            className
          )}
        >
          <Noise />
          {children}
          {/* Buttons centered horizontally */}
          <div className="flex justify-center gap-4 mt-8">
            {/* First Button */}
            <motion.button
              style={{
                position: "relative",
                zIndex: 1000,
                fontFamily: "'Poppins', sans-serif",
              }}
              onClick={(event) => {
                event.stopPropagation();
                console.log("First button clicked");
                handleNavigateDashboard();
              }}
              className="px-6 py-3 bg-blue-500 text-white rounded-lg hover:bg-blue-400 hover:scale-105 hover:shadow-lg transition-all duration-200 ease-in-out"
            >
              <span className="text-lg font-semibold">Go to Dashboard</span>
            </motion.button>
            {/* Second Button */}
            <motion.button
              style={{
                position: "relative",
                zIndex: 1000,
                fontFamily: "'Poppins', sans-serif",
              }}
              onClick={(event) => {
                event.stopPropagation();
                console.log("Second button clicked");
                handleOtherAction();
              }}
              className="px-6 py-3 bg-green-500 text-white rounded-lg hover:bg-green-400 hover:scale-105 hover:shadow-lg transition-all duration-200 ease-in-out"
            >
              <span className="text-lg font-semibold">Second Action</span>
            </motion.button>
          </div>
        </motion.div>
      </div>
    </motion.section>
  );
};

const Noise = () => {
  return (
    <div
      className="absolute inset-0 w-full h-full scale-[1.2] transform opacity-10 [mask-image:radial-gradient(#fff,transparent,75%)]"
      style={{
        backgroundImage: "url(/noise.webp)",
        backgroundSize: "30%",
      }}
    ></div>
  );
};
