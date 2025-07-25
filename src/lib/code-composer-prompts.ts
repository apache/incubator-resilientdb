/**
 * CodeComposer system prompts and templates
 * Implements plan-first chain-of-thought for academic paper to code synthesis
 */

export interface CodeComposerContext {
    language: string;
    scope: string[];
    chunks: string;
    query: string;
    documentTitles: string[];
  }
  
  export interface ProjectStyleGuide {
    language: string;
    conventions: {
      naming: string;
      indentation: string;
      comments: string;
      errorHandling: string;
      imports: string;
    };
    patterns: string[];
  }
  
  const LANGUAGE_STYLE_GUIDES: Record<string, ProjectStyleGuide> = {
    ts: {
      language: "TypeScript",
      conventions: {
        naming: "camelCase for variables/functions, PascalCase for classes/interfaces",
        indentation: "2 spaces",
        comments: "JSDoc for functions, inline // for complex logic",
        errorHandling: "Use Result<T, E> pattern or throw specific Error types",
        imports: "Use ES6 imports, group by: external libs, internal modules, relative imports"
      },
      patterns: [
        "Use interfaces for data contracts",
        "Prefer const assertions for immutable data",
        "Use async/await over Promises.then()",
        "Implement proper error boundaries"
      ]
    },
    go: {
      language: "Go",
      conventions: {
        naming: "camelCase for private, PascalCase for public, use descriptive names",
        indentation: "tabs",
        comments: "Package comments above package declaration, function comments above func",
        errorHandling: "Return error as last return value, check errors explicitly",
        imports: "Standard library first, then external, then internal packages"
      },
      patterns: [
        "Use struct embedding for composition",
        "Implement String() method for custom types",
        "Use channels for goroutine communication",
        "Follow effective Go guidelines"
      ]
    },
    rust: {
      language: "Rust",
      conventions: {
        naming: "snake_case for variables/functions, PascalCase for types/traits",
        indentation: "4 spaces",
        comments: "/// for doc comments, // for inline comments",
        errorHandling: "Use Result<T, E> and Option<T>, avoid unwrap() in production",
        imports: "Use 'use' statements, group by: std, external crates, internal modules"
      },
      patterns: [
        "Prefer owned types over references when possible",
        "Use pattern matching extensively",
        "Implement Display and Debug traits",
        "Follow Rust API guidelines"
      ]
    },
    cpp: {
      language: "C++",
      conventions: {
        naming: "snake_case or camelCase consistently, PascalCase for classes",
        indentation: "2 or 4 spaces consistently",
        comments: "Doxygen-style /** */ for documentation, // for inline",
        errorHandling: "Use exceptions for exceptional cases, error codes for expected failures",
        imports: "#include system headers first, then project headers"
      },
      patterns: [
        "Use RAII for resource management",
        "Prefer smart pointers over raw pointers",
        "Use const correctness throughout",
        "Follow Core Guidelines"
      ]
    }
  };
  
  export const CODE_COMPOSER_SYSTEM_PROMPT = `You are CodeComposer-v0, an AI that transforms academic research into production-ready code.
  
  Your task is to synthesize academic concepts from research papers into practical, runnable implementations.
  
  ## Process (MANDATORY - Follow this exact structure):
  
  ### 1. PLANNING PHASE
  First, create an explicit plan with these sections:
  - **Key Abstractions**: List the main concepts, algorithms, or data structures from the papers
  - **Architecture Overview**: High-level system design and component relationships  
  - **Implementation Strategy**: Step-by-step approach to translate theory to code
  - **Scope Boundaries**: What will be implemented vs. stubbed as TODOs
  
  ### 2. PSEUDOCODE PHASE  
  Create language-agnostic pseudocode that:
  - Captures the core algorithmic logic
  - Shows data flow and control structures
  - Abstracts away language-specific details
  - Includes complexity analysis where relevant
  
  ### 3. IMPLEMENTATION PHASE
  Translate pseudocode to target language with:
  - Clear function signatures and type annotations
  - Comprehensive documentation
  - Error handling and edge cases
  - TODO stubs for non-deterministic or out-of-scope parts
  
  ## Style Guide Adherence:
  Follow the target language's conventions strictly:
  {{STYLE_GUIDE}}
  
  ## Quality Requirements:
  - Code must be syntactically correct and runnable
  - Include necessary imports and dependencies
  - Add meaningful variable names and comments
  - Implement proper error handling
  - Use appropriate data structures and algorithms
  - Consider performance implications
  
  ## Scope Handling:
  - For complex algorithms: Implement core logic, stub advanced optimizations
  - For system architectures: Provide interfaces and key components
  - For mathematical proofs: Implement verifiable computation steps
  - For experimental results: Create reproducible test frameworks
  
  ## Output Format:
  \`\`\`
  ## PLAN
  [Your planning phase here]
  
  ## PSEUDOCODE  
  [Language-agnostic pseudocode here]
  
  ## IMPLEMENTATION
  [Target language implementation here]
  \`\`\`
  
  Remember: Academic papers contain theoretical insights - your job is to make them practically actionable while maintaining scientific rigor.`;
  
  export const generateCodeComposerPrompt = (context: CodeComposerContext): string => {
    const styleGuide = LANGUAGE_STYLE_GUIDES[context.language] || LANGUAGE_STYLE_GUIDES.ts;
    
    const styleGuideText = `
  **${styleGuide.language} Style Guide:**
  - Naming: ${styleGuide.conventions.naming}
  - Indentation: ${styleGuide.conventions.indentation}  
  - Comments: ${styleGuide.conventions.comments}
  - Error Handling: ${styleGuide.conventions.errorHandling}
  - Imports: ${styleGuide.conventions.imports}
  
  **Patterns to Follow:**
  ${styleGuide.patterns.map(pattern => `- ${pattern}`).join('\n')}
  `;
  
    const scopeText = context.scope.length > 0 
      ? `\n**User Scope Focus:** ${context.scope.join(', ')}`
      : '';
  
    return CODE_COMPOSER_SYSTEM_PROMPT.replace('{{STYLE_GUIDE}}', styleGuideText) + `
  
  ## Context Information:
  **Documents:** ${context.documentTitles.join(', ')}
  **Target Language:** ${styleGuide.language}${scopeText}
  
  **Retrieved Academic Content:**
  ${context.chunks}
  
  **User Request:** ${context.query}
  
  Now, following the mandatory 3-phase process (PLAN → PSEUDOCODE → IMPLEMENTATION), synthesize this academic content into production-ready ${styleGuide.language} code.`;
  };
  
  export interface ChainOfThoughtMarkers {
    PLAN_START: string;
    PLAN_END: string;
    PSEUDOCODE_START: string;
    PSEUDOCODE_END: string;
    IMPLEMENTATION_START: string;
    IMPLEMENTATION_END: string;
  }
  
  export const CHAIN_OF_THOUGHT_MARKERS: ChainOfThoughtMarkers = {
    PLAN_START: '## PLAN',
    PLAN_END: '## PSEUDOCODE',
    PSEUDOCODE_START: '## PSEUDOCODE', 
    PSEUDOCODE_END: '## IMPLEMENTATION',
    IMPLEMENTATION_START: '## IMPLEMENTATION',
    IMPLEMENTATION_END: '```'
  };
  
  export interface ParsedResponse {
    plan: string;
    pseudocode: string;
    implementation: string;
    hasStructuredResponse: boolean;
  }
  
  /**
   * Parse chain-of-thought response into structured components
   */
  export const parseChainOfThoughtResponse = (response: string): ParsedResponse => {
    const markers = CHAIN_OF_THOUGHT_MARKERS;
    
    const planStart = response.indexOf(markers.PLAN_START);
    const pseudocodeStart = response.indexOf(markers.PSEUDOCODE_START);
    const implementationStart = response.indexOf(markers.IMPLEMENTATION_START);
    
    if (planStart === -1 || pseudocodeStart === -1 || implementationStart === -1) {
      // Fallback if structure is not followed
      return {
        plan: "Planning phase not found in response",
        pseudocode: "Pseudocode phase not found in response", 
        implementation: response,
        hasStructuredResponse: false
      };
    }
  
    const plan = response.substring(
      planStart + markers.PLAN_START.length,
      pseudocodeStart
    ).trim();
  
    const pseudocode = response.substring(
      pseudocodeStart + markers.PSEUDOCODE_START.length,
      implementationStart  
    ).trim();
  
    const implementation = response.substring(
      implementationStart + markers.IMPLEMENTATION_START.length
    ).trim();
  
    return {
      plan,
      pseudocode,
      implementation,
      hasStructuredResponse: true
    };
  };
  
  /**
   * Extract just the implementation code for file output
   */
  export const extractImplementationCode = (response: string): string => {
    const parsed = parseChainOfThoughtResponse(response);
    
    if (!parsed.hasStructuredResponse) {
      // Try to extract code blocks from unstructured response
      const codeBlockRegex = /```[\w]*\n([\s\S]*?)\n```/g;
      const matches = [...response.matchAll(codeBlockRegex)];
      if (matches && matches.length > 0) {
        return matches[matches.length - 1][1].trim();
      }
      return response;
    }
  
    // Clean up implementation code
    return parsed.implementation
      .replace(/^```[\w]*\n?/, '')
      .replace(/\n?```$/, '')
      .trim();
  };