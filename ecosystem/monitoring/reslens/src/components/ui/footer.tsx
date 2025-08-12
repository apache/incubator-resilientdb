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

import React from 'react';
import { FaLinkedin, FaGithub, FaTwitter } from 'react-icons/fa';
import { ExternalLink } from "lucide-react";

const Footer: React.FC = () => {
  return (
    <footer className="bg-zinc-900 text-gray-300 py-8 font-['Poppins',sans-serif]">
      <div className="container mx-auto px-4">
        <div className="flex flex-col md:flex-row justify-between">
          {/* Left Section: Logo and Description */}
          <div className="mb-6 md:mb-0 flex-1">
            <img
              src="/Reslens.png"  // Changed to PNG logo
              alt="Company Logo"
              className="h-16 mb-10"
            />
            <p className="text-sm text-gray-400 max-w-xs">
            A memory profiling tool for visualizing and analyzing the stats and performance of ResilientDB.
            </p>
          </div>

          {/* Middle Section: Quick Links */}
          <div className="mb-6 md:mb-0 flex-1">
            <h3 className="text-lg font-semibold mb-4 text-white">Quick Links</h3>
            <ul className="space-y-2">
              <li>
                <a
                  href="/"
                  className="hover:text-white transition-colors duration-300 flex items-center"
                >
                  <span className="w-1 h-1 bg-blue-500 rounded-full mr-2"></span>
                  Home
                </a>
              </li>
              <li>
                <a
                  href="https://github.com/Bismanpal-Singh/MemLens"
                  className="hover:text-white transition-colors duration-300 flex items-center"
                >
                  <span className="w-1 h-1 bg-blue-500 rounded-full mr-2"></span>
                  Check out our code
                </a>
              </li>
              <li>
                <a
                  href="/authors"
                  className="hover:text-white transition-colors duration-300 flex items-center"
                >
                  <span className="w-1 h-1 bg-blue-500 rounded-full mr-2"></span>
                  About the team
                </a>
              </li>
              <li>
                <a
                  href="#"
                  className="hover:text-white transition-colors duration-300 flex items-center"
                >
                  <span className="w-1 h-1 bg-blue-500 rounded-full mr-2"></span>
                  Blog
                </a>
              </li>
            </ul>
          </div>

          {/* Right Section: Connect With Us */}
          <div className="flex-1 md:ml-auto md:text-right">
            <h3 className="text-lg font-semibold mb-4 text-white">Connect With Us</h3>
            <div className="flex justify-center md:justify-end space-x-4">
              <a
                href="https://resilientdb.apache.org/"
                className="text-gray-400 hover:text-white transition-colors duration-300 transform hover:scale-110"
              >
                <ExternalLink size={24} />
              </a>
              <a
                href="https://github.com/ResilientEcosystem"
                className="text-gray-400 hover:text-white transition-colors duration-300 transform hover:scale-110"
              >
                <FaGithub size={24} />
              </a>
              <a
                href="#"
                className="text-gray-400 hover:text-white transition-colors duration-300 transform hover:scale-110"
              >
                <FaTwitter size={24} />
              </a>
            </div>
          </div>
        </div>

        {/* Footer Bottom Section */}
        <div className="mt-8 pt-8 border-t border-gray-700 text-center text-sm text-gray-500">
          <p>&copy; {new Date().getFullYear()} ResLens. Built at UC Davis.</p>
        </div>
      </div>
    </footer>
  );
};

export default Footer;
