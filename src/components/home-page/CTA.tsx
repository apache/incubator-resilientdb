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
  const [swipeComplete, setSwipeComplete] = useState(false);

  const navigate = useNavigate();

  const handleMouseMove = (event: React.MouseEvent<HTMLElement>) => {
    const { clientX, clientY } = event;
    const rect = event.currentTarget.getBoundingClientRect();
    const x = (clientX - (rect.left + rect.width / 2)) / 20;
    const y = (clientY - (rect.top + rect.height / 2)) / 20;
    setMousePosition({ x, y });
  };

  const handleSwipeComplete = () => {
    console.log("Navigating to /dashboard");
    setSwipeComplete(true);
    setTimeout(() => navigate("/dashboard"), 300); // Delay for animation
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
          ? `translate3d(${mousePosition.x}px, ${mousePosition.y}px, 0) scale3d(1.03, 1.03, 1) rotate3d(1, 1, 0, 5deg)`
          : "translate3d(0px, 0px, 0) scale3d(1, 1, 1) rotate3d(0, 0, 0, 0deg)",
        transition: "transform 0.2s ease-out",
      }}
      className={cn(
        "mx-auto w-full bg-gradient-to-r from-gray-800 via-gray-700 to-gray-800 relative rounded-2xl overflow-hidden shadow-xl",
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
            transform: isHovering
              ? `translate3d(${-mousePosition.x}px, ${-mousePosition.y}px, 0) scale3d(1.05, 1.05, 1)`
              : "translate3d(0px, 0px, 0) scale3d(1, 1, 1)",
            transition: "transform 0.2s ease-out",
          }}
          className={cn(
            "h-full px-6 py-16 sm:px-10 text-gray-200 font-sans tracking-wide",
            className
          )}
        >
          <Noise />
          {children}
          {/* Swipe to Unlock */}
          <div className="mt-8 relative w-full h-16 bg-gray-700 rounded-full overflow-hidden">
            <motion.div
              drag="x"
              dragConstraints={{ left: 0, right: 300 }} // Adjust based on width
              onDragEnd={(e, info) => {
                if (info.offset.x > 250) handleSwipeComplete(); // Trigger navigation
              }}
              className="absolute top-0 left-0 w-16 h-16 bg-blue-600 rounded-full cursor-pointer flex items-center justify-center"
              style={{ zIndex: 1000 }}
              whileHover={{ scale: 1.1 }}
              whileTap={{ scale: 0.9 }}
            >
              <span className="text-white font-bold text-lg">→</span>
            </motion.div>
            <p className="absolute inset-0 flex items-center justify-center text-white font-semibold text-lg">
                Move to Dashboard
            </p>
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
