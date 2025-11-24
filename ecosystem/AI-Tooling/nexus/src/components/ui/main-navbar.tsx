"use client";

import { cn } from "@/lib/utils";
import { motion } from "framer-motion";
import { Home, MessageSquare, Settings } from "lucide-react";
import Link from "next/link";
import { usePathname } from "next/navigation";





interface MainNavBarProps {
  className?: string;
}

export function MainNavBar({ className }: MainNavBarProps) {
  const pathname = usePathname();



  const navItems = [
    { name: "Home", url: "/", icon: Home },
    { name: "Chat", url: "/chat", icon: MessageSquare },
    { name: "Researcher", url: "/research", icon: Settings },
  ];





  const isResearcher = pathname === "/research";
  return (
    <div
      className={cn("fixed top-0 left-0 w-full py-4 px-4 z-50", className)}
      hidden={isResearcher}
    >
      {/* <div className="flex gap-3">
            <Image src="/resdb-logo.svg" alt="ResDB Logo" width={100} height={100} />
            <Image src="/expolab-icon.png" alt="ExpoLab Icon" width={100} height={100} />
        </div> */}
      <div className="flex items-center justify-center gap-3 bg-background/5 border border-border backdrop-blur-lg py-1 px-1 rounded-full shadow-lg w-fit mx-auto">
        {navItems.map((item) => {
          const Icon = item.icon;
          const isActive = pathname === item.url;

          return (
            <Link
              key={item.name}
              href={item.url}
              className={cn(
                "relative cursor-pointer text-sm font-semibold px-6 py-2 rounded-full transition-colors",
                "text-foreground/80 hover:text-primary",
                isActive && "bg-muted text-primary",
              )}
            >
              <span className="hidden md:inline">{item.name}</span>
              <span className="md:hidden">
                <Icon size={18} strokeWidth={2.5} />
              </span>
              {isActive && (
                <motion.div
                  layoutId="lamp"
                  className="absolute inset-0 w-full bg-primary/5 rounded-full -z-10"
                  initial={false}
                  transition={{
                    type: "spring",
                    stiffness: 300,
                    damping: 30,
                  }}
                >
                  <div className="absolute -top-2 left-1/2 -translate-x-1/2 w-8 h-1 bg-primary rounded-t-full">
                    <div className="absolute w-12 h-6 bg-primary/20 rounded-full blur-md -top-2 -left-2" />
                    <div className="absolute w-8 h-6 bg-primary/20 rounded-full blur-md -top-1" />
                    <div className="absolute w-4 h-4 bg-primary/20 rounded-full blur-sm top-0 left-2" />
                  </div>
                </motion.div>
              )}
            </Link>
          );
        })}
      </div>
    </div>
  );
}
