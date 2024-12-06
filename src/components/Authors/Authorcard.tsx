"use client";

import { CardBody, CardContainer, CardItem } from "@/components/Authors/3d-card";
import { Github, Linkedin } from 'lucide-react';

interface AuthorCardProps {
  name: string;
  description: string;
  imageSrc: string;
  LinkedinHandle: string;
  GithubHandle: string;
  title: string;  // New prop for the author's title
}

export function AuthorCard({ name, description, imageSrc, LinkedinHandle, GithubHandle, title }: AuthorCardProps) {
  return (
    <CardContainer className="inter-var">
      <CardBody className="bg-gray-50 relative group/card dark:hover:shadow-2xl dark:hover:shadow-emerald-500/[0.1] dark:bg-black dark:border-white/[0.2] border-black/[0.1] w-auto sm:w-[30rem] h-auto rounded-xl p-6 border">
        <CardItem
          translateZ="50"
          className="text-xl font-bold text-neutral-600 dark:text-white"
        >
          {name}
        </CardItem>
        <CardItem
          as="p"
          translateZ="60"
          className="text-neutral-500 text-sm max-w-sm mt-2 dark:text-neutral-300"
        >
          {description}
        </CardItem>
        <CardItem translateZ="100" className="w-full mt-4">
          <img
            src={imageSrc}
            height="1000"
            width="1000"
            className="h-60 w-full object-cover rounded-xl group-hover/card:shadow-xl"
            alt={`${name}'s portrait`}
          />
        </CardItem>
        <div className="flex justify-between items-center mt-4">
          <CardItem
            translateZ={20}
            className="flex items-center space-x-2"
          >
            <span className="text-lg font-semibold dark:text-white">{title}</span>
            <a
              href={`https://github.com/${GithubHandle}`}
              target="_blank"
              rel="noopener noreferrer"
              className="p-2 rounded-full bg-gray-100 dark:bg-gray-800 hover:bg-gray-200 dark:hover:bg-gray-700 transition-colors"
            >
              <Github className="w-5 h-5 text-gray-600 dark:text-gray-300" />
            </a>
          </CardItem>
          <CardItem
            translateZ={20}
            as="a"
            href={`https://www.linkedin.com/in/${LinkedinHandle}`}
            target="_blank"
            rel="noopener noreferrer"
            className="flex items-center space-x-2 px-4 py-2 rounded-xl text-sm font-medium bg-opacity-10 bg-blue-500 text-blue-600 dark:text-blue-400 hover:bg-opacity-20 transition-colors"
          >
            <Linkedin className="w-5 h-5" />
            <span>Follow on LinkedIn</span>
          </CardItem>
        </div>
      </CardBody>
    </CardContainer>
  );
}
