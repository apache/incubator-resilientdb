export const config = {
  // Database configuration
  databaseUrl: process.env.DATABASE_URL || "",

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
};
