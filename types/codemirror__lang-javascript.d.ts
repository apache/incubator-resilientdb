declare module '@codemirror/lang-javascript' {
  import { Extension } from '@codemirror/state';
  
  export function javascript(options?: {
    typescript?: boolean;
    jsx?: boolean;
  }): Extension;
} 