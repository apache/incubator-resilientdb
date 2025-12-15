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

import dotenv from "dotenv";
dotenv.config();

export const config = {
  // Database configuration
  databaseUrl: process.env.DATABASE_URL || "",
  embedDim: parseInt(process.env.EMBEDDING_DIM || "768"),

  // DeepSeek LLM configuration
  deepSeekApiKey: process.env.DEEPSEEK_API_KEY || "",
  deepSeekModel: (process.env.DEEPSEEK_MODEL || "deepseek-chat") as
    | "deepseek-chat"
    | "deepseek-coder",

  // LlamaCloud configuration
  llamaCloudApiKey: process.env.LLAMA_CLOUD_API_KEY || "",
  llamaCloudProjectName: process.env.LLAMA_CLOUD_PROJECT_NAME || "",
  llamaCloudIndexName: process.env.LLAMA_CLOUD_INDEX_NAME || "",
  llamaCloudBaseUrl: process.env.LLAMA_CLOUD_BASE_URL || "",

  tavilyApiKey: process.env.TAVILY_API_KEY || "",

  // Gemini configuration
  geminiApiKey: process.env.GEMINI_API_KEY || "",

  // Supabase configuration
  supabaseUrl: process.env.SUPABASE_URL || "",
  supabaseKey:
    process.env.SUPABASE_SERVICE_ROLE_KEY ||
    process.env.SUPABASE_ANON_KEY ||
    "",
  supabaseVectorTable: process.env.SUPABASE_VECTOR_TABLE || "llamaindex_vector_embeddings",
  supabaseMemoryTable:
    process.env.SUPABASE_MEMORY_TABLE || "llamaindex_memory_embeddings",
};
