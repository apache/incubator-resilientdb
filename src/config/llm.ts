import env from './environment';

// Re-export env for Hugging Face check
const HUGGINGFACE_API_KEY = env.HUGGINGFACE_API_KEY;

/**
 * LLM Configuration
 * 
 * Centralized configuration for LLM providers
 */
export interface LLMConfig {
  provider: 'deepseek' | 'openai' | 'anthropic' | 'huggingface' | 'local';
  apiKey?: string;
  model: string;
  enableLiveStats: boolean;
}

/**
 * Get LLM configuration from environment
 */
export function getLLMConfig(): LLMConfig {
  // For Hugging Face, use HUGGINGFACE_API_KEY if LLM_API_KEY is not set
  let apiKey = env.LLM_API_KEY;
  if (env.LLM_PROVIDER === 'huggingface' && !apiKey && HUGGINGFACE_API_KEY) {
    apiKey = HUGGINGFACE_API_KEY;
  }
  
  return {
    provider: env.LLM_PROVIDER,
    apiKey: apiKey,
    model: env.LLM_MODEL || (
      env.LLM_PROVIDER === 'huggingface'
        ? 'meta-llama/Meta-Llama-3-8B-Instruct'
        : env.LLM_PROVIDER === 'local'
          ? 'Xenova/distilgpt2'
          : 'deepseek-chat'
    ),
    enableLiveStats: env.LLM_ENABLE_LIVE_STATS,
  };
}

/**
 * Check if LLM is configured and available
 */
export function isLLMAvailable(): boolean {
  const config = getLLMConfig();
  if (config.provider === 'local') {
    return true;
  }
  // For Hugging Face, we can use HUGGINGFACE_API_KEY if LLM_API_KEY is not set
  if (config.provider === 'huggingface') {
    return (config.apiKey !== undefined && config.apiKey.length > 0) ||
           (env.HUGGINGFACE_API_KEY !== undefined && env.HUGGINGFACE_API_KEY.length > 0);
  }
  // For other providers, API key is required
  return config.apiKey !== undefined && config.apiKey.length > 0;
}

