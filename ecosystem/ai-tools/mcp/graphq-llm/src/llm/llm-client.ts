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

import OpenAI from 'openai';
import Anthropic from '@anthropic-ai/sdk';
import { GoogleGenerativeAI } from '@google/generative-ai';
import { HfInference } from '@huggingface/inference';
import { getLLMConfig, isLLMAvailable } from '../config/llm';
import env from '../config/environment';

/**
 * LLM Client
 * 
 * Unified interface for multiple LLM providers:
 * - OpenAI (via OpenAI SDK)
 * - Anthropic (via Anthropic SDK)
 * - DeepSeek (via OpenAI-compatible API)
 * - Hugging Face (via Inference API)
 * 
 * Supports Live Stats integration for enhanced context
 */
export interface LLMMessage {
  role: 'system' | 'user' | 'assistant';
  content: string;
}

export interface LLMResponse {
  content: string;
  model: string;
  usage?: {
    promptTokens?: number;
    completionTokens?: number;
    totalTokens?: number;
  };
}

export interface LLMOptions {
  temperature?: number;
  maxTokens?: number;
  liveStats?: Record<string, unknown>; // Live stats from ResLens (if enabled)
}

type LocalGenerationPipeline = (prompt: string, options?: Record<string, unknown>) => Promise<any>;

export class LLMClient {
  private openaiClient: OpenAI | null = null;
  private anthropicClient: Anthropic | null = null;
  private geminiClient: GoogleGenerativeAI | null = null;
  private huggingfaceClient: HfInference | null = null;
  private provider: 'deepseek' | 'openai' | 'anthropic' | 'huggingface' | 'local' | 'gemini';
  private model: string;
  private enableLiveStats: boolean;
  private huggingfaceApiKey?: string;
  private localPipelinePromise?: Promise<LocalGenerationPipeline>;

  constructor() {
    const config = getLLMConfig();
    this.provider = config.provider;
    this.model = config.model;
    this.enableLiveStats = config.enableLiveStats;

    // Initialize provider-specific clients
    if (config.apiKey) {
      if (this.provider === 'openai' || this.provider === 'deepseek') {
        // DeepSeek uses OpenAI-compatible API
        this.openaiClient = new OpenAI({
          apiKey: config.apiKey,
          baseURL: this.provider === 'deepseek' 
            ? 'https://api.deepseek.com/v1' 
            : undefined,
        });
      } else if (this.provider === 'anthropic') {
        this.anthropicClient = new Anthropic({
          apiKey: config.apiKey,
        });
      } else if (this.provider === 'gemini') {
        this.geminiClient = new GoogleGenerativeAI(config.apiKey);
      } else if (this.provider === 'huggingface') {
        this.huggingfaceApiKey = config.apiKey;
        this.huggingfaceClient = new HfInference(config.apiKey);
      }
    } else if (this.provider === 'local') {
      this.huggingfaceApiKey = config.apiKey || env.HUGGINGFACE_API_KEY;
      this.localPipelinePromise = this.initLocalPipeline();
    }
  }

  /**
   * Check if LLM client is available
   */
  isAvailable(): boolean {
    if (this.provider === 'local') {
      return true;
    }
    return isLLMAvailable() && (
      this.openaiClient !== null || 
      this.anthropicClient !== null ||
      this.geminiClient !== null ||
      this.huggingfaceClient !== null
    );
  }

  /**
   * Get the current provider
   */
  getProvider(): string {
    return this.provider;
  }

  /**
   * Get the current model
   */
  getModel(): string {
    return this.model;
  }

  /**
   * Generate completion using the configured LLM
   * 
   * @param messages - Array of messages (system, user, assistant)
   * @param options - LLM options (temperature, maxTokens, liveStats)
   * @returns Promise<LLMResponse>
   */
  async generateCompletion(
    messages: LLMMessage[],
    options: LLMOptions = {}
  ): Promise<LLMResponse> {
    if (!this.isAvailable()) {
      throw new Error(
        `LLM client not available. Please set LLM_API_KEY environment variable for provider: ${this.provider}`
      );
    }

    // Enhance messages with live stats if enabled
    const enhancedMessages = this.enableLiveStats && options.liveStats
      ? this.enhanceMessagesWithLiveStats(messages, options.liveStats)
      : messages;

    try {
      if (this.provider === 'openai' || this.provider === 'deepseek') {
        return this.generateOpenAICompletion(enhancedMessages, options);
      } else if (this.provider === 'anthropic') {
        return this.generateAnthropicCompletion(enhancedMessages, options);
      } else if (this.provider === 'gemini') {
        return this.generateGeminiCompletion(enhancedMessages, options);
      } else if (this.provider === 'huggingface') {
        return this.generateHuggingFaceCompletion(enhancedMessages, options);
      } else if (this.provider === 'local') {
        return this.generateLocalCompletion(enhancedMessages, options);
      } else {
        throw new Error(`Unsupported LLM provider: ${this.provider}`);
      }
    } catch (error) {
      // Retry logic for transient errors
      if (this.isRetryableError(error)) {
        console.warn(`LLM request failed, retrying... Error: ${error instanceof Error ? error.message : String(error)}`);
        // Wait a bit before retry
        await new Promise(resolve => setTimeout(resolve, 1000));
        return this.generateCompletion(messages, options);
      }
      throw error;
    }
  }

  /**
   * Generate completion using OpenAI/DeepSeek API
   */
  private async generateOpenAICompletion(
    messages: LLMMessage[],
    options: LLMOptions
  ): Promise<LLMResponse> {
    if (!this.openaiClient) {
      throw new Error('OpenAI/DeepSeek client not initialized');
    }

    const response = await this.openaiClient.chat.completions.create({
      model: this.model,
      messages: messages.map(msg => ({
        role: msg.role,
        content: msg.content,
      })),
      temperature: options.temperature ?? 0.7,
      max_tokens: options.maxTokens,
    });

    const choice = response.choices[0];
    if (!choice || !choice.message) {
      throw new Error('No completion returned from LLM');
    }

    return {
      content: choice.message.content || '',
      model: response.model,
      usage: response.usage ? {
        promptTokens: response.usage.prompt_tokens,
        completionTokens: response.usage.completion_tokens,
        totalTokens: response.usage.total_tokens,
      } : undefined,
    };
  }

  /**
   * Generate completion using Anthropic API
   */
  private async generateAnthropicCompletion(
    messages: LLMMessage[],
    options: LLMOptions
  ): Promise<LLMResponse> {
    if (!this.anthropicClient) {
      throw new Error('Anthropic client not initialized');
    }

    // Anthropic requires system message to be separate
    const systemMessages = messages.filter(m => m.role === 'system');
    const systemMessage = systemMessages.length > 0 
      ? systemMessages.map(m => m.content).join('\n\n')
      : '';
    const conversationMessages = messages.filter(m => m.role !== 'system');

    const response = await this.anthropicClient.messages.create({
      model: this.model,
      max_tokens: options.maxTokens ?? 1024,
      temperature: options.temperature ?? 0.7,
      system: systemMessage || undefined,
      messages: conversationMessages.map(msg => ({
        role: msg.role === 'user' ? 'user' : 'assistant',
        content: msg.content,
      })),
    });

    const content = response.content[0];
    if (!content || content.type !== 'text') {
      throw new Error('No text completion returned from Anthropic');
    }

    return {
      content: content.text,
      model: response.model,
      usage: response.usage ? {
        promptTokens: response.usage.input_tokens,
        completionTokens: response.usage.output_tokens,
        totalTokens: response.usage.input_tokens + response.usage.output_tokens,
      } : undefined,
    };
  }

  /**
   * Generate completion using Google Gemini API
   */
  private async generateGeminiCompletion(
    messages: LLMMessage[],
    options: LLMOptions
  ): Promise<LLMResponse> {
    if (!this.geminiClient) {
      throw new Error('Gemini client not initialized');
    }

    // Get the model
    const model = this.geminiClient.getGenerativeModel({ 
      model: this.model,
      generationConfig: {
        temperature: options.temperature ?? 0.7,
        maxOutputTokens: options.maxTokens,
      },
    });

    // Separate system message from conversation
    const systemMessages = messages.filter(m => m.role === 'system');
    const systemMessage = systemMessages.length > 0 
      ? systemMessages.map(m => m.content).join('\n\n')
      : '';
    
    const conversationMessages = messages.filter(m => m.role !== 'system');

    // Build chat history for multi-turn conversations
    const history: Array<{ role: string; parts: Array<{ text: string }> }> = [];
    for (let i = 0; i < conversationMessages.length - 1; i++) {
      const msg = conversationMessages[i];
      history.push({
        role: msg.role === 'user' ? 'user' : 'model',
        parts: [{ text: msg.content }],
      });
    }

    // Get the last message (user's current question)
    const lastMessage = conversationMessages[conversationMessages.length - 1];
    
    let result;
    if (history.length > 0) {
      // Use chat session for multi-turn conversations
      const chat = model.startChat({
        history: history,
        systemInstruction: systemMessage || undefined,
      });
      result = await chat.sendMessage(lastMessage.content);
    } else {
      // Use simple generateContent for single-turn
      let prompt = lastMessage.content;
      if (systemMessage) {
        prompt = `${systemMessage}\n\n${prompt}`;
      }
      result = await model.generateContent(prompt);
    }

    const response = await result.response;
    const text = response.text();

    // Get token usage if available
    const usageMetadata = response.usageMetadata;
    const promptTokens = usageMetadata?.promptTokenCount ?? Math.ceil(
      messages.map(m => m.content).join('\n').length / 4
    );
    const completionTokens = usageMetadata?.candidatesTokenCount ?? Math.ceil(text.length / 4);

    return {
      content: text,
      model: this.model,
      usage: {
        promptTokens,
        completionTokens,
        totalTokens: promptTokens + completionTokens,
      },
    };
  }

  /**
   * Generate completion using Hugging Face Inference API
   */
  private async generateHuggingFaceCompletion(
    messages: LLMMessage[],
    options: LLMOptions
  ): Promise<LLMResponse> {
    if (!this.huggingfaceClient) {
      throw new Error('Hugging Face client not initialized');
    }

    // Convert messages to Hugging Face format
    // Hugging Face text generation expects a single prompt string
    let prompt = '';
    for (const msg of messages) {
      if (msg.role === 'system') {
        prompt += `System: ${msg.content}\n\n`;
      } else if (msg.role === 'user') {
        prompt += `User: ${msg.content}\n\n`;
      } else if (msg.role === 'assistant') {
        prompt += `Assistant: ${msg.content}\n\n`;
      }
    }
    prompt += 'Assistant:';

    try {
      // Use direct HTTP API to bypass SDK's Inference Providers feature
      // This uses the router endpoint directly (similar to embeddings)
      const url = `https://router.huggingface.co/hf-inference/models/${this.model}`;
      
      const headers: Record<string, string> = {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${this.huggingfaceApiKey}`,
      };

      // Format messages for text generation
      // For chat models, we'll use a simple prompt format
      const requestBody = {
        inputs: prompt,
        parameters: {
          max_new_tokens: options.maxTokens || 500,
          temperature: options.temperature || 0.7,
          return_full_text: false,
        },
      };

      const response = await fetch(url, {
        method: 'POST',
        headers,
        body: JSON.stringify(requestBody),
      });

      if (!response.ok) {
        const errorText = await response.text();
        let errorMessage = `HTTP ${response.status}: ${errorText}`;
        
        if (response.status === 503) {
          errorMessage = `Model is loading, please wait a few seconds and try again: ${errorText}`;
        } else if (response.status === 429) {
          errorMessage = `Rate limit exceeded: ${errorText}`;
        } else if (response.status === 401 || response.status === 403) {
          errorMessage = `Authentication failed: ${errorText}`;
        }
        
        throw new Error(`Hugging Face API error: ${errorMessage}`);
      }

      const data = await response.json();
      
      // Extract generated text
      let generatedText = '';
      if (Array.isArray(data) && data.length > 0) {
        generatedText = data[0].generated_text || '';
      } else if (typeof data === 'object' && 'generated_text' in data) {
        generatedText = (data as { generated_text: string }).generated_text;
      } else if (typeof data === 'string') {
        generatedText = data;
      }

      // Estimate token usage (rough estimate)
      const promptTokens = Math.ceil(prompt.length / 4);
      const completionTokens = Math.ceil(generatedText.length / 4);

      return {
        content: generatedText.trim(),
        model: this.model,
        usage: {
          promptTokens,
          completionTokens,
          totalTokens: promptTokens + completionTokens,
        },
      };
    } catch (error) {
      const errorMessage = error instanceof Error ? error.message : String(error);
      
      // Handle specific error cases
      if (errorMessage.includes('503') || errorMessage.includes('loading')) {
        throw new Error(`Model is loading, please wait a few seconds and try again: ${errorMessage}`);
      } else if (errorMessage.includes('429') || errorMessage.includes('rate limit')) {
        throw new Error(`Rate limit exceeded: ${errorMessage}`);
      } else if (errorMessage.includes('401') || errorMessage.includes('403') || errorMessage.includes('authentication')) {
        throw new Error(`Authentication failed: ${errorMessage}`);
      }
      
      throw new Error(`Hugging Face API error: ${errorMessage}`);
    }
  }

  /**
   * Initialize local text-generation pipeline
   */
  private async initLocalPipeline(): Promise<LocalGenerationPipeline> {
    if (this.huggingfaceApiKey) {
      if (!process.env.HF_TOKEN) {
        process.env.HF_TOKEN = this.huggingfaceApiKey;
      }
      if (!process.env.HF_HUB_READ_TOKEN) {
        process.env.HF_HUB_READ_TOKEN = this.huggingfaceApiKey;
      }
    }

    const transformers = await import('@xenova/transformers');
    const pipeline = await transformers.pipeline('text-generation', this.model);
    return pipeline;
  }

  /**
   * Generate completion using local @xenova/transformers pipeline
   */
  private async generateLocalCompletion(
    messages: LLMMessage[],
    options: LLMOptions
  ): Promise<LLMResponse> {
    if (!this.localPipelinePromise) {
      this.localPipelinePromise = this.initLocalPipeline();
    }
    const pipeline = await this.localPipelinePromise;

    let prompt = '';
    for (const msg of messages) {
      if (msg.role === 'system') {
        prompt += `System: ${msg.content}\n\n`;
      } else if (msg.role === 'user') {
        prompt += `User: ${msg.content}\n\n`;
      } else if (msg.role === 'assistant') {
        prompt += `Assistant: ${msg.content}\n\n`;
      }
    }
    prompt += 'Assistant:';

    const generated = await pipeline(prompt, {
      max_new_tokens: options.maxTokens ?? 400,
      temperature: options.temperature ?? 0.7,
      return_full_text: false,
    });

    let generatedText = '';
    if (Array.isArray(generated) && generated.length > 0) {
      generatedText = generated[0].generated_text || generated[0].output_text || '';
    } else if (generated && typeof generated === 'object') {
      generatedText =
        (generated as Record<string, unknown>).generated_text as string ||
        (generated as Record<string, unknown>).output_text as string ||
        '';
    }

    const promptTokens = Math.ceil(prompt.length / 4);
    const completionTokens = Math.ceil(generatedText.length / 4);

    return {
      content: generatedText.trim(),
      model: this.model,
      usage: {
        promptTokens,
        completionTokens,
        totalTokens: promptTokens + completionTokens,
      },
    };
  }

  /**
   * Enhance messages with live stats from ResLens
   */
  private enhanceMessagesWithLiveStats(
    messages: LLMMessage[],
    liveStats: Record<string, unknown>
  ): LLMMessage[] {
    // Add live stats as context in the system message
    const statsContext = `\n\n[Live Query Statistics]\n${JSON.stringify(liveStats, null, 2)}`;
    
    // Find or create system message
    const systemMessageIndex = messages.findIndex(m => m.role === 'system');
    if (systemMessageIndex >= 0) {
      // Append to existing system message
      const enhancedMessages = [...messages];
      enhancedMessages[systemMessageIndex] = {
        ...enhancedMessages[systemMessageIndex],
        content: enhancedMessages[systemMessageIndex].content + statsContext,
      };
      return enhancedMessages;
    } else {
      // Add new system message with live stats
      return [
        {
          role: 'system',
          content: `You have access to live query statistics from ResLens. Use this information to provide more accurate and context-aware responses.${statsContext}`,
        },
        ...messages,
      ];
    }
  }

  /**
   * Check if error is retryable
   */
  private isRetryableError(error: unknown): boolean {
    if (error instanceof Error) {
      const message = error.message.toLowerCase();
      // Retry on network errors, rate limits, and server errors
      return (
        message.includes('network') ||
        message.includes('timeout') ||
        message.includes('rate limit') ||
        message.includes('429') ||
        message.includes('500') ||
        message.includes('503')
      );
    }
    return false;
  }

  /**
   * Simple chat completion (convenience method)
   */
  async chat(
    userMessage: string,
    systemMessage?: string,
    options: LLMOptions = {}
  ): Promise<string> {
    const messages: LLMMessage[] = [];
    
    if (systemMessage) {
      messages.push({
        role: 'system',
        content: systemMessage,
      });
    }
    
    messages.push({
      role: 'user',
      content: userMessage,
    });

    const response = await this.generateCompletion(messages, options);
    return response.content;
  }
}

