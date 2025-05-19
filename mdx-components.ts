import { useMDXComponents as getDocsMDXComponents } from 'nextra-theme-docs';
import PythonSplitIDE from '@/components/PythonIDE'
import { PythonPlayground } from '@/components/PythonPlayground';

const docsComponents = getDocsMDXComponents();

export const useMDXComponents = (components?: any): any => ({
  ...docsComponents,
  PythonSplitIDE,
  PythonPlayground,
  ...components,
});