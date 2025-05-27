declare module 'react-markdown' {
  import { ComponentType, ReactNode } from 'react';

  interface ReactMarkdownProps {
    children: string;
    components?: {
      [key: string]: ComponentType<any>;
    };
  }

  const ReactMarkdown: ComponentType<ReactMarkdownProps>;
  export default ReactMarkdown;
} 