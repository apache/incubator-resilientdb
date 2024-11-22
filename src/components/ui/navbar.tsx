import React, { useState } from 'react';
import { Menu, X } from 'lucide-react';

const Navbar: React.FC = () => {
  const [isOpen, setIsOpen] = useState(false);

  const toggleMenu = () => {
    setIsOpen(!isOpen);
  };

  return (
    <nav className="bg-black shadow-lg fixed top-0 left-0 w-full z-50">
      <div className="max-w-6xl mx-auto px-4">
        <div className="flex justify-between">
          <div className="flex space-x-7">
            <div>
              <a href="#" className="flex items-center py-4 px-2">
                <span className="font-semibold text-gray-500 text-lg">Logo</span>
              </a>
            </div>
          </div>
          <div className="hidden md:flex items-center space-x-1">
            <a
              href="#"
              className="py-4 px-2 text-gray-500 font-semibold hover:text-green-500 transition duration-300"
            >
              Home
            </a>
            <a
              href="#"
              className="py-4 px-2 text-gray-500 font-semibold hover:text-green-500 transition duration-300"
            >
              Services
            </a>
            <a
              href="#"
              className="py-4 px-2 text-gray-500 font-semibold hover:text-green-500 transition duration-300"
            >
              About
            </a>
            <a
              href="#"
              className="py-4 px-2 text-gray-500 font-semibold hover:text-green-500 transition duration-300"
            >
              Contact
            </a>
          </div>
          <div className="md:hidden flex items-center">
            <button className="outline-none mobile-menu-button" onClick={toggleMenu}>
              {isOpen ? <X size={24} /> : <Menu size={24} />}
            </button>
          </div>
        </div>
      </div>
      <div className={`md:hidden ${isOpen ? 'block' : 'hidden'}`}>
        <a href="#" className="block py-2 px-4 text-sm hover:bg-gray-200">Home</a>
        <a href="#" className="block py-2 px-4 text-sm hover:bg-gray-200">Services</a>
        <a href="#" className="block py-2 px-4 text-sm hover:bg-gray-200">About</a>
        <a href="#" className="block py-2 px-4 text-sm hover:bg-gray-200">Contact</a>
      </div>
    </nav>
  );
};

export default Navbar;
