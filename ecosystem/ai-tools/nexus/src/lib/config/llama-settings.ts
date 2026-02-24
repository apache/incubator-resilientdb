/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

import { config } from "@/config/environment";
import { DeepSeekLLM } from "@llamaindex/deepseek";
import { GEMINI_EMBEDDING_MODEL, GeminiEmbedding } from "@llamaindex/google";
import chalk from "chalk";
import { LLMMetadata, Settings } from "llamaindex";

// Track if settings have been configured to prevent duplicate setup
let isConfigured = false;

/**
 * Configure LlamaIndex global settings for DeepSeek LLM and Gemini embeddings
 * This centralizes all LLM and embedding configuration for the application
 */
export const configureLlamaSettings = (): void => {
  // Prevent duplicate configuration
  if (isConfigured) {
    return;
  }

  console.log(chalk.bold("[LlamaSettings] Configuring LlamaIndex with DeepSeek + Gemini"));

  /**
   * Custom DeepSeek LLM with corrected metadata
   */
  class DeepSeekLLMWithMetadata extends DeepSeekLLM {
    get metadata(): LLMMetadata {
      return {
        contextWindow: 64_000,
        model: this.model,
        temperature: this.temperature,
        topP: this.topP,
        tokenizer: 'cl100k_base' as any,
        structuredOutput: true,
      };
    }
  }
  Settings.llm = new DeepSeekLLMWithMetadata({
    apiKey: config.deepSeekApiKey,
    model: "deepseek-chat",
    timeout: 30000,
    maxRetries: 2,
    temperature: 0.1,
  });
  

  Settings.embedModel = new GeminiEmbedding({
    model: GEMINI_EMBEDDING_MODEL.EMBEDDING_001,
    apiKey: config.geminiApiKey,
  });

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