import { PGVectorStore } from "@llamaindex/postgres";
import chalk from "chalk";
import { StorageContext, storageContextFromDefaults } from "llamaindex";
import { config } from "../config/environment";

class VectorStoreService {
  private static instance: VectorStoreService;
  private vectorStore: PGVectorStore | null = null;
  private storageContext: StorageContext | null = null;

  private constructor() {}

  static getInstance(): VectorStoreService {
    if (!VectorStoreService.instance) {
      VectorStoreService.instance = new VectorStoreService();
    }
    return VectorStoreService.instance;
  }

  // Initialize the PGVectorStore with configuration
  private async initializeVectorStore(): Promise<PGVectorStore> {
    if (this.vectorStore) {
      return this.vectorStore;
    }

    try {
      console.log(chalk.blue("[VectorStoreService] Initializing PGVectorStore..."));

      // Log the configuration being used
      const vectorStoreConfig = {
        clientConfig: {
          database: config.vectorStore.database,
          host: config.vectorStore.host,
          port: config.vectorStore.port,
          user: config.vectorStore.user,
          password: config.vectorStore.password,
        },
        tableName: config.vectorStore.tableName,
        dimensions: config.vectorStore.embedDim,
        performSetup: true, // Enable automatic setup of vector extension and tables
      };
      
      console.log(chalk.blue("[VectorStoreService] PGVectorStore configuration:"));
      console.log(chalk.gray(JSON.stringify({
        ...vectorStoreConfig,
        clientConfig: {
          ...vectorStoreConfig.clientConfig,
          password: "***hidden***"
        }
      }, null, 2)));

      // Create PGVectorStore with configuration from environment
      this.vectorStore = new PGVectorStore(vectorStoreConfig);

      console.log(chalk.green("[VectorStoreService] PGVectorStore initialized successfully"));
      return this.vectorStore;
    } catch (error) {
      console.error(chalk.red("[VectorStoreService] Failed to initialize PGVectorStore:"), error);
      throw new Error(`Vector store initialization failed: ${error instanceof Error ? error.message : String(error)}`);
    }
  }

  // Get or create the StorageContext with PGVectorStore
  async getStorageContext(): Promise<StorageContext> {
    if (this.storageContext) {
      return this.storageContext;
    }

    try {
      const vectorStore = await this.initializeVectorStore();
      
      // Create StorageContext with the PGVectorStore
      this.storageContext = await storageContextFromDefaults({
        vectorStore: vectorStore,
      });

      console.log(chalk.green("[VectorStoreService] StorageContext created with PGVectorStore"));
      return this.storageContext;
    } catch (error) {
      console.error(chalk.red("[VectorStoreService] Failed to create StorageContext:"), error);
      throw new Error(`Storage context creation failed: ${error instanceof Error ? error.message : String(error)}`);
    }
  }

  // Get the vector store instance directly (for advanced usage)
  async getVectorStore(): Promise<PGVectorStore> {
    return await this.initializeVectorStore();
  }

  // Set collection for multi-tenancy support
  async setCollection(collectionName: string): Promise<void> {
    try {
      const vectorStore = await this.initializeVectorStore();
      vectorStore.setCollection(collectionName);
      console.log(chalk.blue(`[VectorStoreService] Collection set to: ${collectionName}`));
    } catch (error) {
      console.error(chalk.red("[VectorStoreService] Failed to set collection:"), error);
      throw error;
    }
  }

  // Get current collection name
  async getCollection(): Promise<string> {
    try {
      const vectorStore = await this.initializeVectorStore();
      return vectorStore.getCollection();
    } catch (error) {
      console.error(chalk.red("[VectorStoreService] Failed to get collection:"), error);
      throw error;
    }
  }

  // Health check for vector store connection
  async healthCheck(): Promise<boolean> {
    try {
      const vectorStore = await this.initializeVectorStore();
      // Try to perform a simple operation to verify connection
      await vectorStore.getCollection();
      console.log(chalk.green("[VectorStoreService] Health check passed"));
      return true;
    } catch (error) {
      console.error(chalk.red("[VectorStoreService] Health check failed:"), error);
      return false;
    }
  }

  // Clear the vector store instances (useful for testing or reconfiguration)
  clearInstances(): void {
    this.vectorStore = null;
    this.storageContext = null;
    console.log(chalk.yellow("[VectorStoreService] Instances cleared"));
  }
}

// Export singleton instance
export const vectorStoreService = VectorStoreService.getInstance();