'use client';

import { useEffect, useRef } from 'react';

declare global {
  interface Window {
    initializeChatbot: (config: any) => void;
  }
}

interface PocketFlowChatbotProps {
  extraUrls?: string[];
  prefixes?: string[];
  chatbotName?: string;
  wsUrl?: string;
  instruction?: string;
  isOpen?: boolean;
}

export function PocketFlowChatbot({
  extraUrls = ["https://beacon.resilientdb.com/docs"],
  prefixes = [],
  chatbotName = 'Beacon',
  wsUrl = 'wss://askthispage.com/api/ws/chat',
  instruction = '',
  isOpen = false
}: PocketFlowChatbotProps) {
  const containerRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    // Check if the script is already loaded
    if (document.querySelector('script[src="https://askthispage.com/embed/chatbot.js"]')) {
      return;
    }

    const script = document.createElement("script");
    script.src = "https://askthispage.com/embed/chatbot.js";
    script.onload = function() {
      if (window.initializeChatbot) {
        window.initializeChatbot({
          extra_urls: extraUrls,
          prefixes: prefixes,
          chatbotName: chatbotName,
          wsUrl: wsUrl,
          instruction: instruction,
          isOpen: isOpen
        });
      }
    };
    document.head.appendChild(script);

    // Cleanup function
    return () => {
      const existingScript = document.querySelector('script[src="https://askthispage.com/embed/chatbot.js"]');
      if (existingScript) {
        existingScript.remove();
      }
    };
  }, [extraUrls, prefixes, chatbotName, wsUrl, instruction, isOpen]);

  // Add custom styles after the chatbot loads
  useEffect(() => {
    const checkAndStyle = () => {
      const chatbot = document.querySelector('.askthispage-chatbot') as HTMLElement;
      if (chatbot) {
        // Move to left corner
        chatbot.style.position = 'fixed';
        chatbot.style.left = '20px';
        chatbot.style.right = 'auto';
        chatbot.style.bottom = '20px';
        chatbot.style.zIndex = '1000';

        // Add experimental badge
        const badge = document.createElement('div');
        badge.innerHTML = '🧪 Experimental';
        badge.style.cssText = `
          position: absolute;
          top: -30px;
          left: 0;
          background: linear-gradient(45deg, #ff6b6b, #ffa500);
          color: white;
          padding: 4px 8px;
          border-radius: 12px;
          font-size: 10px;
          font-weight: bold;
          text-transform: uppercase;
          letter-spacing: 0.5px;
          box-shadow: 0 2px 8px rgba(0,0,0,0.2);
          z-index: 1001;
          animation: pulse 2s infinite;
          white-space: nowrap;
        `;

        // Add pulse animation
        const style = document.createElement('style');
        style.textContent = `
          @keyframes pulse {
            0% { opacity: 1; }
            50% { opacity: 0.7; }
            100% { opacity: 1; }
          }
        `;
        document.head.appendChild(style);

        chatbot.appendChild(badge);
      }
    };

    // Check immediately and then periodically
    checkAndStyle();
    const interval = setInterval(checkAndStyle, 1000);

    return () => {
      clearInterval(interval);
    };
  }, []);

  return (
    <div 
      ref={containerRef}
      style={{
        position: 'fixed',
        left: '20px',
        bottom: '20px',
        zIndex: 1000,
        pointerEvents: 'none'
      }}
    />
  );
} 