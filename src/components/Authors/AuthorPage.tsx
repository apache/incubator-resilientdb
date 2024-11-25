"use client"

import React from "react"
import Navbar from "@/components/ui/navbar"
import { AuthorCard } from "@/components/Authors/Authorcard"
import Footer from "../ui/footer"

const authors = [
  {
    name: "Harish Krishnakumar Gokul",
    description: "Bestselling author of science fiction novels, known for her intricate world-building and compelling characters.",
    imageSrc: "H.jpeg",
    LinkedinHandle: "harish-gokul01"
  },
  {
    name: "Bismanpal Singh Anand",
    description: "Award-winning journalist and non-fiction writer, specializing in investigative reporting and social issues.",
    imageSrc: "try.JPG",
    LinkedinHandle: "bismanpal-singh"
  },
  {
    name: "Georgy Zeats",
    description: "Poet and essayist, her work explores themes of identity, culture, and the immigrant experience in America.",
    imageSrc: "G.jpg",
    LinkedinHandle: "georgy-zaets"
  },
  {
    name: "Krishna Karthik",
    description: "Historical fiction author, bringing forgotten stories from the past to life with meticulous research and vivid prose.",
    imageSrc: "K.jpg",
    LinkedinHandle: "krishna-karthik-97b74b222"
  }
]

export default function AuthorsPage() {
  return (
    <div className="min-h-screen" style={{ backgroundColor: '#0c0c0c' }}>
      <Navbar />
      <main className="container mx-auto px-4 py-8 pt-20"> {/* Added pt-20 for top padding */}
        <h1 className="text-3xl font-bold text-center mb-8 text-gray-800 dark:text-white">
          The Geeks.
        </h1>
        <div className="grid grid-cols-1 md:grid-cols-2 gap-8">
          {authors.map((author, index) => (
            <div key={index} className="flex justify-center">
              <AuthorCard {...author} />
            </div>
          ))}
        </div>
      </main>
      <Footer/>
    </div>
  )
}

