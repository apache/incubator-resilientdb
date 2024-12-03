import { Card, CardContent } from "@/components/ui/card"
import { useState } from "react"
import { motion } from "framer-motion"
import { Info } from 'lucide-react'
import {
  Tooltip,
  TooltipContent,
  TooltipProvider,
  TooltipTrigger,
} from "@/components/ui/tooltip"

const Odometer = ({ value }) => {
  const angle = (value / 100) * 180

  return (
    <div className="relative w-full h-24 mx-auto mb-2">
      <svg viewBox="0 0 100 50" className="w-full h-full" preserveAspectRatio="xMidYMid meet">
        <path
          d="M 10 50 A 40 40 0 0 1 90 50"
          fill="none"
          stroke="url(#gradient)"
          strokeWidth="10"
          strokeLinecap="round"
        />
        <defs>
          <linearGradient id="gradient" x1="0%" y1="0%" x2="100%" y2="0%">
            <stop offset="0%" stopColor="#10B981" />
            <stop offset="50%" stopColor="#FBBF24" />
            <stop offset="100%" stopColor="#EF4444" />
          </linearGradient>
        </defs>
        <motion.path
          d="M 50 50 L 50 15 L 52 15 L 50 10 L 48 15 L 50 15 Z"
          fill="white"
          initial={{ rotate: 0 }}
          animate={{ rotate: angle }}
          transition={{ type: "spring", stiffness: 50 }}
          style={{ transformOrigin: '50px 50px' }}
        />
      </svg>
    </div>
  )
}

export function CacheHitOdometers() {
  return (
    <div className="grid grid-cols-3 gap-4">
      {[1, 2, 3].map((index) => {
        const value = Math.floor(Math.random() * 100)
        return (
          <Card key={index} className="bg-slate-800">
            <CardContent className="p-4">
              <Odometer value={value} />
              <div className="text-2xl font-bold text-center">{value}%</div>
              <div className="text-sm text-center text-slate-400">Cache Hit {index}</div>
            </CardContent>
          </Card>
        )
      })}
    </div>
  )
}