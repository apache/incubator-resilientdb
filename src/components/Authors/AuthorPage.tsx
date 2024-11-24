"use client"

import React from "react"
import Navbar from "@/components/ui/navbar"
import { AuthorCard } from "@/components/Authors/Authorcard"

const authors = [
  {
    name: "Jane Doe",
    description: "Bestselling author of science fiction novels, known for her intricate world-building and compelling characters.",
    imageSrc: "B.JPG",
    twitterHandle: "janedoe_author"
  },
  {
    name: "John Smith",
    description: "Award-winning journalist and non-fiction writer, specializing in investigative reporting and social issues.",
    imageSrc: "B.JPG",
    twitterHandle: "johnsmith_writes"
  },
  {
    name: "Emily Chen",
    description: "Poet and essayist, her work explores themes of identity, culture, and the immigrant experience in America.",
    imageSrc: "B.JPG",
    twitterHandle: "emilychen_poet"
  },
  {
    name: "Michael Brown",
    description: "Historical fiction author, bringing forgotten stories from the past to life with meticulous research and vivid prose.",
    imageSrc: "B.JPG",
    twitterHandle: "mbrown_history"
  }
]

export default function AuthorsPage() {
  return (
    <div className="min-h-screen bg-gray-100 dark:bg-gray-900">
      <Navbar />
      <main className="container mx-auto px-4 py-8 pt-20"> {/* Added pt-20 for top padding */}
        <h1 className="text-3xl font-bold text-center mb-8 text-gray-800 dark:text-white">
          Our Authors
        </h1>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
          {authors.map((author, index) => (
            <div key={index} className="flex justify-center">
              <AuthorCard {...author} />
            </div>
          ))}
        </div>
      </main>
    </div>
  )
}

