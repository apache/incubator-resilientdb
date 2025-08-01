import dotenv from "dotenv";
dotenv.config();

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

  // Vector Storage configuration
  vectorStore: {
    // Use the same database connection for vector storage
    database: process.env.POSTGRES_DATABASE || process.env.DATABASE_URL?.split('/').pop()?.split('?')[0] || "nexus",
    host: process.env.POSTGRES_HOST || process.env.DATABASE_URL?.split('@')[1]?.split(':')[0] || "localhost",
    port: parseInt(process.env.POSTGRES_PORT || process.env.DATABASE_URL?.split(':')[2]?.split('/')[0] || "5432"),
    user: process.env.POSTGRES_USER || process.env.DATABASE_URL?.split('://')[1]?.split('@')[0] || "postgres",
    password: process.env.POSTGRES_PASSWORD || (process.env.DATABASE_URL?.includes(':') && process.env.DATABASE_URL?.includes('@') ? process.env.DATABASE_URL.split('://')[1]?.split(':')[1]?.split('@')[0] : "") || "",
    tableName: process.env.VECTOR_TABLE_NAME || "document_embeddings",
    embedDim: parseInt(process.env.EMBEDDING_DIM || "1536"), // Use EMBEDDING_DIM from .env
    // HNSW configuration for performance tuning
    hnswConfig: {
      m: parseInt(process.env.HNSW_M || "16"),
      efConstruction: parseInt(process.env.HNSW_EF_CONSTRUCTION || "64"),
      efSearch: parseInt(process.env.HNSW_EF_SEARCH || "40"),
      distMethod: process.env.HNSW_DIST_METHOD || "vector_cosine_ops"
    }
  }
};
