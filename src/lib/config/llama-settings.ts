import { config } from "@/config/environment";
import { DeepSeekLLM } from "@llamaindex/deepseek";
import { HuggingFaceEmbedding } from "@llamaindex/huggingface";
import chalk from "chalk";
import { Settings } from "llamaindex";

// Track if settings have been configured to prevent duplicate setup
let isConfigured = false;

/**
 * Configure LlamaIndex global settings for DeepSeek LLM and HuggingFace embeddings
 * This centralizes all LLM and embedding configuration for the application
 */
export const configureLlamaSettings = (): void => {
  // Prevent duplicate configuration
  if (isConfigured) {
    return;
  }

  console.log(chalk.bold("[LlamaSettings] Configuring LlamaIndex with DeepSeek + HuggingFace"));

  // Configure DeepSeek LLM with aggressive timeout and retry settings
  Settings.llm = new DeepSeekLLM({
    apiKey: config.deepSeekApiKey,
    model: config.deepSeekModel,
    timeout: 30000, // 30 second timeout
    maxRetries: 2,
  });

  // Configure HuggingFace embedding model
  // Using default model for faster setup (can upgrade later)
  Settings.embedModel = new HuggingFaceEmbedding();


  // Mark as configured
  isConfigured = true;
  console.log(chalk.bold("[LlamaSettings] LlamaIndex settings configured"));
};

/**
 * Get the current LLM instance
 * Useful for creating agents with specific LLM configurations
 */
export const getCurrentLLM = () => {
  if (!Settings.llm && !isConfigured) {
    configureLlamaSettings();
  }
  return Settings.llm;
};

/**
 * Get the current embedding model
 * Useful for custom retrieval implementations
 */
export const getCurrentEmbedModel = () => {
  if (!Settings.embedModel && !isConfigured) {
    configureLlamaSettings();
  }
  return Settings.embedModel;
};

/**
 * Reset settings (useful for testing)
 */
export const resetLlamaSettings = (): void => {
  // Clear existing settings
  Settings.llm = undefined as any;
  Settings.embedModel = undefined as any;

  // Reset configuration flag
  isConfigured = false;

  // Reconfigure
  configureLlamaSettings();
};