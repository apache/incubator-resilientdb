declare module 'react-syntax-highlighter' {
  import { ComponentType } from 'react';

  interface SyntaxHighlighterProps {
    language?: string;
    style?: any;
    children?: string;
    PreTag?: string | ComponentType<any>;
    [key: string]: any;
  }

  export const Prism: ComponentType<SyntaxHighlighterProps>;
}

declare module 'react-syntax-highlighter/dist/cjs/styles/prism' {
  const vscDarkPlus: any;
  export { vscDarkPlus };
}
