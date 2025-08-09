import { BaseMemoryBlock, Settings } from "llamaindex";
// had to extend this since the original class doesn't parse json markdown
type RobustFactBlockOptions = {
  id?: string;
  priority?: number;
  isLongTerm?: boolean;
  llm?: any;
  maxFacts: number;
  extractionPrompt?: string;
  summaryPrompt?: string;
};

const DEFAULT_EXTRACTION_PROMPT = `
You are a precise fact extraction system designed to identify key information from conversations.

CONVERSATION SEGMENT:
{{conversation}}

EXISTING FACTS:
{{existing_facts}}

INSTRUCTIONS: 
1. Review the conversation segment provided above.
2. Extract specific, concrete facts the user has disclosed or important information discovered
3. Focus on factual information like preferences, personal details, requirements, constraints, or context
4. Do not include opinions, summaries, or interpretations - only extract explicit information
5. Do not duplicate facts that are already in the existing facts list

Respond with the new facts from the conversation segment using the following JSON format:
{
  "facts": ["fact1", "fact2", "fact3", ...]
}
`;

const DEFAULT_SUMMARY_PROMPT = `
You are a precise fact condensing system designed to summarize facts in a concise manner.

EXISTING FACTS:
{{existing_facts}}

INSTRUCTIONS:
1. Review the current list of existing facts
2. Condense the facts into a more concise list, less than {{ max_facts }} facts
3. Focus on factual information like preferences, personal details, requirements, constraints, or context
4. Do not include opinions, summaries, or interpretations - only extract explicit information
5. Do not duplicate facts that are already in the existing facts list

Respond with the condensed facts using the following JSON format:
{
  "facts": ["fact1", "fact2", "fact3", ...]
}
`;

function stripCodeFences(text: string): string {
  return text
    .replace(/```[a-zA-Z0-9]*\s*/g, "")
    .replace(/```/g, "")
    .trim();
}

function extractFirstJson(text: string): any {
  const cleaned = stripCodeFences(text);
  try {
    return JSON.parse(cleaned);
  } catch {}

  const firstBrace = cleaned.indexOf("{");
  const lastBrace = cleaned.lastIndexOf("}");
  if (firstBrace !== -1 && lastBrace !== -1 && lastBrace > firstBrace) {
    const candidate = cleaned.substring(firstBrace, lastBrace + 1);
    try {
      return JSON.parse(candidate);
    } catch {}
  }

  const firstBracket = cleaned.indexOf("[");
  const lastBracket = cleaned.lastIndexOf("]");
  if (firstBracket !== -1 && lastBracket !== -1 && lastBracket > firstBracket) {
    const candidate = cleaned.substring(firstBracket, lastBracket + 1);
    try {
      return JSON.parse(candidate);
    } catch {}
  }
  throw new Error("Unable to parse JSON from LLM output");
}

export class RobustFactExtractionMemoryBlock extends BaseMemoryBlock {
  private facts: string[] = [];
  private llm: any;
  private maxFacts: number;
  private extractionPrompt: string;
  private summaryPrompt: string;

  constructor(options: RobustFactBlockOptions) {
    super({ id: options.id, priority: options.priority ?? 1, isLongTerm: options.isLongTerm ?? true });
    this.llm = options.llm ?? Settings.llm;
    this.maxFacts = options.maxFacts;
    this.extractionPrompt = options.extractionPrompt ?? DEFAULT_EXTRACTION_PROMPT;
    this.summaryPrompt = options.summaryPrompt ?? DEFAULT_SUMMARY_PROMPT;
  }

  async get(_messages?: any): Promise<any[]> {
    return [
      {
        id: this.id,
        content: this.facts.join("\n"),
        role: "system" as any,
      } as any,
    ];
  }

  async put(messages: any[]): Promise<void> {
    if (messages.length === 0) return;

    const existingFactsStr = `{ facts: [${this.facts.join(", ")}] }`;
    const conversation = `\n\t${messages.map((m) => m.content).join("\n\t")}`;
    const prompt = this.extractionPrompt
      .replace("{{conversation}}", conversation)
      .replace("{{existing_facts}}", existingFactsStr);

    const response = await this.llm.complete({ prompt });
    const parsed = extractFirstJson(response.text);
    if (!parsed || !Array.isArray(parsed.facts)) return;
    if (parsed.facts.length > 0) {
      this.facts.push(...parsed.facts);
    }

    if (this.facts.length > this.maxFacts) {
      const existingFactsStr2 = `{ facts: [${this.facts.join(", ")}] }`;
      const sumPrompt = this.summaryPrompt
        .replace("{{existing_facts}}", existingFactsStr2)
        .replace("{{max_facts}}", String(this.maxFacts));
      const sumRes = await this.llm.complete({ prompt: sumPrompt });
      const condensed = extractFirstJson(sumRes.text);
      if (condensed && Array.isArray(condensed.facts) && condensed.facts.length > 0) {
        this.facts = condensed.facts.slice(0, this.maxFacts);
      }
    }
  }
}

export const robustFactExtractionBlock = (options: RobustFactBlockOptions) =>
  new RobustFactExtractionMemoryBlock(options);


