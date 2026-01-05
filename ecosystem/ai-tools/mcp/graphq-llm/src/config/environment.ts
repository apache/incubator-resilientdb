import { z } from 'zod';
import dotenv from 'dotenv';

dotenv.config();

const envSchema = z.object({
  // ResilientDB Configuration
  // GraphQL server runs on port 5001 (KV service is on port 18000)
  RESILIENTDB_GRAPHQL_URL: z.string().url().default('http://localhost:5001/graphql'),
  RESILIENTDB_API_KEY: z.string().optional(),
  // Default keys for transactions (can be overridden)
  RESILIENTDB_SIGNER_PUBLIC_KEY: z.string().optional(),
  RESILIENTDB_SIGNER_PRIVATE_KEY: z.string().optional(),
  RESILIENTDB_RECIPIENT_PUBLIC_KEY: z.string().optional(),

  // Nexus Configuration
  // Nexus runs on port 3000 by default (Next.js app)
  // API endpoint: POST /api/research/chat
  NEXUS_API_URL: z.string().url().default('http://localhost:3000'),
  NEXUS_API_KEY: z.string().optional(), // Optional - Nexus may not require API key

  // ResLens Configuration
  // NOTE: ResLens API endpoints not yet defined - these are placeholders
  // Currently using local in-memory storage, so API key is NOT needed
  RESLENS_API_URL: z.string().url().optional().or(z.literal('').transform(() => undefined)),
  RESLENS_API_KEY: z.string().optional().or(z.literal('').transform(() => undefined)), // NOT NEEDED - only for future API integration
  // Live Mode: Enable real-time stats polling for LLM access
  RESLENS_LIVE_MODE: z.coerce.boolean().default(false),
  RESLENS_POLL_INTERVAL: z.coerce.number().default(5000), // milliseconds

  // MCP Server Configuration
  MCP_SERVER_PORT: z.coerce.number().default(3001),
  MCP_SERVER_HOST: z.string().default('localhost'),

  // LLM Configuration
  LLM_PROVIDER: z.enum(['deepseek', 'openai', 'anthropic', 'huggingface', 'local', 'gemini']).default('deepseek'),
  LLM_API_KEY: z.string().optional(),
  LLM_MODEL: z.string().default('deepseek-chat'),
  // Enable Live Stats access for LLM (requires ResLens Live Mode)
  LLM_ENABLE_LIVE_STATS: z.coerce.boolean().default(false),
  
  // Embedding Provider Configuration
  EMBEDDINGS_PROVIDER: z.enum(['huggingface', 'local']).default('huggingface'),
  HUGGINGFACE_API_KEY: z.string().optional(),

  // Database (optional)
  DATABASE_URL: z.string().optional(),

  // Logging
  LOG_LEVEL: z.enum(['debug', 'info', 'warn', 'error']).default('info'),
});

export type Environment = z.infer<typeof envSchema>;

let env: Environment;

try {
  env = envSchema.parse(process.env);
} catch (error) {
  if (error instanceof z.ZodError) {
    console.error('Environment validation error:', error.errors);
    process.exit(1);
  }
  throw error;
}

export default env;

