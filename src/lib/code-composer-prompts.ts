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

export const LANGUAGE_STYLE_GUIDES: Record<string, ProjectStyleGuide> = {
  ts: {
    language: "TypeScript",
    conventions: {
      naming:
        "camelCase for vars & functions; PascalCase for classes, types, enums; ALL_CAPS for env constants",
      indentation: "2 spaces (enforced by prettier)",
      comments:
        "JSDoc for public symbols; inline // only when the intent is non-obvious; keep ≤80 chars",
      errorHandling:
        "Never swallow errors; prefer Result<T, E> or throw typed error classes; avoid `async void` fire-and-forget",
      imports:
        "Use absolute paths (ts-config baseUrl=src). Order: node built-ins, 3rd-party, internal; one empty line between groups"
    },
    patterns: [
      "Prefer readonly and const assertions for immutability",
      "Use async/await over .then/.catch",
      "Functional utils: map/filter/reduce instead of imperative loops where readable",
      "All new modules need accompanying *.spec.ts tests (jest)",
      "Enable TS strict mode (noImplicitAny, exactOptionalPropertyTypes)",
      "Use eslint-plugin-@typescript-eslint + prettier via pre-commit hook"
    ]
  },

  python: {
    language: "Python",
    conventions: {
      naming:
        "snake_case for vars & functions; PascalCase for classes & exceptions; CAPS for module-level constants",
      indentation: "4 spaces (black auto-formats)",
      comments:
        'PEP 257 docstrings: """Summary line.""", blank line, details; inline # only when needed',
      errorHandling:
        "Specific exceptions; never bare except; use contextlib.suppress only for targeted errors",
      imports:
        "PEP 8 order (stdlib, 3rd-party, local) separated by blank lines; absolute imports preferred"
    },
    patterns: [
      "Type hints everywhere; run mypy in CI with --strict",
      "Use dataclasses or attrs for simple payload objects",
      "Favor list/dict comprehensions; guard against large in-memory builds by using generators",
      "Implement __str__/__repr__; make classes hashable if they’ll be dict keys",
      "Log via structlog (already present in /scripts) – never print() in library code",
      "pytest-style fixtures; 90 % line coverage gate in CI"
    ]
  },

  cpp: {
    language: "C++17",
    conventions: {
      naming:
        "snake_case for functions & variables; PascalCase for classes, enums; UPPER_SNAKE for macros & constants",
      indentation: "2 spaces (clang-format, Google style)",
      comments:
        "Doxygen /** … */ for public API; // for local logic; keep comments in header, not impl, when possible",
      errorHandling:
        "Use std::optional / std::expected (folly::Expected in current code) for recoverable paths; throw only for truly exceptional cases",
      imports:
        "#include <system> first, then 3rd-party, then project; always include what you use; use forward decls in headers when practical"
    },
    patterns: [
      "RAII for every resource (no naked new/delete)",
      "Prefer std::unique_ptr / std::shared_ptr; use gsl::not_null for raw pointer params",
      "Mark everything that can be constexpr as constexpr",
      "Pass small POD by value, larger objects by const&",
      "Use enum class not plain enums",
      "Run clang-tidy with google-readability* and cppcoreguidelines* checks in CI",
      "All headers need #pragma once and the Apache license banner"
    ]
  },
};


export const CODE_COMPOSER_SYSTEM_PROMPT = `You are CodeComposer-v0, an AI that transforms academic research into production-ready code.
  
  Your task is to synthesize academic concepts from research papers into practical, runnable implementations.
  
  ## Process (MANDATORY - Follow this exact structure):
  
  ### 0. TOPIC PHASE
  Start with a concise topic header (2-4 words) that captures the high-level subject matter you'll be implementing.
  
  ### 1. PLANNING PHASE
  Create an explicit plan with these sections:
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
  
  **CRITICAL**: Pseudocode section must be wrapped in triple backticks (\`\`\`) and contain ONLY algorithmic logic in plain code format. 
  NO prose, NO explanatory text, NO markdown formatting. Use comments for clarification only.
  
  ### 3. IMPLEMENTATION PHASE
  Translate pseudocode to target language with:
  - Clear function signatures and type annotations
  - Comprehensive documentation
  - Error handling and edge cases
  - TODO stubs for non-deterministic or out-of-scope parts
  
  **CRITICAL**: Implementation section must be wrapped in triple backticks (\`\`\`) with language identifier and contain ONLY executable code in the target language.
  NO prose, NO explanatory paragraphs, NO markdown formatting. Use code comments for documentation only.
  
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
  
  ## Output Format Requirements:
  
  **TOPIC Section**: Short, high-level subject matter (2-4 words)
  **PLAN Section**: Natural language explanation and planning
  **PSEUDOCODE Section**: Must be wrapped in \`\`\` - pure algorithmic pseudocode only, no prose, no explanations
  **IMPLEMENTATION Section**: Must be wrapped in \`\`\`language - pure target language code only, no prose, no explanations
  
  \`\`\`
  ## TOPIC
  [2-4 word topic description]
  
  ## PLAN
  [Natural language planning and explanation here]
  
  ## PSEUDOCODE  
  \`\`\`
  [PURE PSEUDOCODE ONLY - no prose, no explanations, just algorithmic logic]
  \`\`\`
  
  ## IMPLEMENTATION
  \`\`\`[language]
  [PURE CODE ONLY - no prose, no explanations, just executable code with comments]
  \`\`\` 
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
  TOPIC_START: string;
  TOPIC_END: string;
  PLAN_START: string;
  PLAN_END: string;
  PSEUDOCODE_START: string;
  PSEUDOCODE_END: string;
  IMPLEMENTATION_START: string;
  IMPLEMENTATION_END: string;
}

export const CHAIN_OF_THOUGHT_MARKERS: ChainOfThoughtMarkers = {
  TOPIC_START: '## TOPIC',
  TOPIC_END: '## PLAN',
  PLAN_START: '## PLAN',
  PLAN_END: '## PSEUDOCODE',
  PSEUDOCODE_START: '## PSEUDOCODE',
  PSEUDOCODE_END: '## IMPLEMENTATION',
  IMPLEMENTATION_START: '## IMPLEMENTATION',
  IMPLEMENTATION_END: '```'
};

export interface ParsedResponse {
  topic: string;
  plan: string;
  pseudocode: string;
  implementation: string;
  hasStructuredResponse: boolean;
}


export const parseChainOfThoughtResponse = (response: string): ParsedResponse => {
  const markers = CHAIN_OF_THOUGHT_MARKERS;

  const topicStart = response.indexOf(markers.TOPIC_START);
  const planStart = response.indexOf(markers.PLAN_START);
  const pseudocodeStart = response.indexOf(markers.PSEUDOCODE_START);
  const implementationStart = response.indexOf(markers.IMPLEMENTATION_START);

  if (topicStart === -1 || planStart === -1 || pseudocodeStart === -1 || implementationStart === -1) {
    // fallback if structure is not followed
    return {
      topic: "Implementation",
      plan: "Planning phase not found in response",
      pseudocode: "Pseudocode phase not found in response",
      implementation: response,
      hasStructuredResponse: false
    };
  }

  const topic = response.substring(
    topicStart + markers.TOPIC_START.length,
    planStart
  ).trim();

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
    topic: topic || "Implementation",
    plan: plan,
    pseudocode: ensureCodeBlock(pseudocode),
    implementation: ensureCodeBlock(implementation),
    hasStructuredResponse: true
  };
};


const ensureCodeBlock = (content: string): string => {
  const trimmed = content.trim();

  if (trimmed.startsWith('```') && trimmed.endsWith('```')) {
    return trimmed;
  }

  if (trimmed.startsWith('```') && !trimmed.endsWith('```')) {
    return `${trimmed}\n\`\`\``;
  }

  if (!trimmed.includes('```')) {
    return `\`\`\`\n${trimmed}\n\`\`\``;
  }

  return trimmed;
};
