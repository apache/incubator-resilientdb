/*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
*/

import React, { useEffect, useRef, useState } from "react";
import { AnimatePresence, motion } from "framer-motion";
import { Link } from "react-router-dom";

// Utility function to replace Next.js cn
const cn = (...classes: string[]) => classes.filter(Boolean).join(" ");

export const GlowingStarsBackgroundCard = ({
  className,
  children,
}: {
  className?: string;
  children?: React.ReactNode;
}) => {
  const [mouseEnter, setMouseEnter] = useState(false);

  return (
    <div
      onMouseEnter={() => {
        setMouseEnter(true);
      }}
      onMouseLeave={() => {
        setMouseEnter(false);
      }}
      className={cn(
        "bg-[linear-gradient(110deg,#333_0.6%,#222)] p-4 max-w-4xl h-[20rem] w-full rounded-xl border border-[#eaeaea] dark:border-neutral-600",
        className || ""
      )}
    >
      <div className="flex justify-center items-center">
        <Illustration mouseEnter={mouseEnter} />
      </div>
      <div className="px-2 pb-6">{children}</div>
    </div>
  );
};

export const GlowingStarsDescription = ({
  className,
  children,
}: {
  className?: string;
  children?: React.ReactNode;
}) => {
  return (
    <p className={cn("text-base text-white max-w-[24rem]", className || "")}>
      {children}
    </p>
  );
};

export const GlowingStarsTitle = ({
  className,
  children,
}: {
  className?: string;
  children?: React.ReactNode;
}) => {
  return (
    <h2 className={cn("font-bold text-2xl text-[#eaeaea]", className || "")}>
      {children}
    </h2>
  );
};

export const Illustration = ({ mouseEnter }: { mouseEnter: boolean }) => {
  const stars = 108;
  const columns = 18;

  const [glowingStars, setGlowingStars] = useState<number[]>([]);

  const highlightedStars = useRef<number[]>([]);

  useEffect(() => {
    const interval = setInterval(() => {
      highlightedStars.current = Array.from({ length: 5 }, () =>
        Math.floor(Math.random() * stars)
      );
      setGlowingStars([...highlightedStars.current]);
    }, 3000);

    return () => clearInterval(interval);
  }, []);

  return (
    <div
      className="h-48 p-1 w-full"
      style={{
        display: "grid",
        gridTemplateColumns: `repeat(${columns}, 1fr)`,
        gap: `1px`,
      }}
    >
      {[...Array(stars)].map((_, starIdx) => {
        const isGlowing = glowingStars.includes(starIdx);
        const delay = (starIdx % 10) * 0.1;
        const staticDelay = starIdx * 0.01;
        return (
          <div
            key={`matrix-col-${starIdx}}`}
            className="relative flex items-center justify-center"
          >
            <Star
              isGlowing={mouseEnter ? true : isGlowing}
              delay={mouseEnter ? staticDelay : delay}
            />
            {mouseEnter && <Glow delay={staticDelay} />}
            <AnimatePresence mode="wait">
              {isGlowing && <Glow delay={delay} />}
            </AnimatePresence>
          </div>
        );
      })}
    </div>
  );
};

const Star = ({ isGlowing, delay }: { isGlowing: boolean; delay: number }) => {
  return (
    <motion.div
      key={delay}
      initial={{
        scale: 1,
      }}
      animate={{
        scale: isGlowing ? [1, 1.2, 2.5, 2.2, 1.5] : 1,
        background: isGlowing ? "#fff" : "#666",
      }}
      transition={{
        duration: 2,
        ease: "easeInOut",
        delay: delay,
      }}
      className={cn("bg-[#666] h-[1px] w-[1px] rounded-full relative z-20")}
    ></motion.div>
  );
};

const Glow = ({ delay }: { delay: number }) => {
  return (
    <motion.div
      initial={{
        opacity: 0,
      }}
      animate={{
        opacity: 1,
      }}
      transition={{
        duration: 2,
        ease: "easeInOut",
        delay: delay,
      }}
      exit={{
        opacity: 0,
      }}
      className="absolute left-1/2 -translate-x-1/2 z-10 h-[4px] w-[4px] rounded-full bg-blue-500 blur-[1px] shadow-2xl shadow-blue-400"
    />
  );
};

export function GlowingStarsBackgroundCardPreview() {
  return (
    <div className="flex py-20 items-center justify-center antialiased">
      <GlowingStarsBackgroundCard>
        <GlowingStarsTitle>Explore the Dashboard!</GlowingStarsTitle>
        <div className="flex justify-between items-end">
          <GlowingStarsDescription>
            Head into world of Statistics and Data through the Memory Lens.
          </GlowingStarsDescription>
          <Link
            to="/dashboard"
            className="px-4 py-2 bg-white text-black rounded-md hover:bg-black hover:text-white transition-colors"
          >
            Enter MemLens
          </Link>
        </div>
      </GlowingStarsBackgroundCard>
    </div>
  );
}
