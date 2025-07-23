#!/usr/bin/env tsx
import { readdir, readFile, rm, stat } from "fs/promises";
import { join } from "path";
import { config } from "../src/config/environment";
import { databaseService, DocumentData } from "../src/lib/database";

interface FileMigrationResult {
  documentPath: string;
  success: boolean;
  error?: string;
  documentsCount?: number;
}

class PostgreSQLMigrationScript {
  private parsedDir = join(process.cwd(), "documents", "parsed");

  async run(): Promise<void> {
    console.log("Starting PostgreSQL Migration");
    console.log("=====================================");

    if (!config.databaseUrl) {
      console.error("DATABASE_URL environment variable is required");
      console.log("Please set DATABASE_URL in your .env file or environment");
      process.exit(1);
    }

    try {
      console.log("Initializing database schema...");
      await databaseService.initializeSchema();
      console.log("Database schema initialized");

      const isHealthy = await databaseService.healthCheck();
      if (!isHealthy) {
        throw new Error("Database connection failed");
      }
      console.log("Database connection verified");

      console.log("Scanning for existing parsed files...");
      const migrationResults = await this.migrateParsedFiles();

      this.displayMigrationResults(migrationResults);

      if (migrationResults.length > 0 && migrationResults.every(r => r.success)) {
        console.log("Migration successful! Cleaning up parsed files...");
        await this.cleanupParsedFiles();
        console.log("Parsed files cleanup completed");
      }

      console.log("Migration completed successfully!");
      console.log("=====================================");
    } catch (error) {
      console.error("Migration failed:", error);
      process.exit(1);
    } finally {
      await databaseService.close();
    }
  }

  private async migrateParsedFiles(): Promise<FileMigrationResult[]> {
    const results: FileMigrationResult[] = [];

    try {
      let parsedDirExists = true;
      try {
        await stat(this.parsedDir);
      } catch {
        parsedDirExists = false;
      }

      if (!parsedDirExists) {
        console.log("No parsed files directory found - nothing to migrate");
        return results;
      }

      const directories = await readdir(this.parsedDir);
      console.log(`Found ${directories.length} parsed document directories`);

      for (const dirName of directories) {
        const documentDir = join(this.parsedDir, dirName);
        const documentsPath = join(documentDir, "documents.json");
        const metadataPath = join(documentDir, "metadata.json");

        try {
          const [documentsData, metadataData] = await Promise.all([
            readFile(documentsPath, "utf-8"),
            readFile(metadataPath, "utf-8")
          ]);

          const documents: DocumentData[] = JSON.parse(documentsData);
          const metadata = JSON.parse(metadataData);

          const documentPath = metadata.documentPath || `documents/${dirName}.pdf`;

          console.log(`Migrating: ${documentPath} (${documents.length} chunks)`);

          await databaseService.saveParsedDocuments(
            documentPath,
            documents,
            metadata.originalFileSize || 0,
            new Date(metadata.originalModifiedTime || new Date())
          );

          results.push({
            documentPath,
            success: true,
            documentsCount: documents.length
          });

          console.log(`Successfully migrated: ${documentPath}`);
        } catch (error) {
          const documentPath = `documents/${dirName}.pdf`;
          console.error(`Failed to migrate ${documentPath}:`, error);
          results.push({
            documentPath,
            success: false,
            error: error instanceof Error ? error.message : String(error)
          });
        }
      }
    } catch (error) {
      console.error("Error scanning parsed files:", error);
    }

    return results;
  }

  private displayMigrationResults(results: FileMigrationResult[]): void {
    console.log("\nMigration Results");
    console.log("====================");

    if (results.length === 0) {
      console.log("No files were migrated");
      return;
    }

    const successful = results.filter(r => r.success);
    const failed = results.filter(r => !r.success);

    console.log(`Successful: ${successful.length}`);
    console.log(`Failed: ${failed.length}`);
    console.log(`Total: ${results.length}`);

    if (successful.length > 0) {
      console.log("\nSuccessfully migrated:");
      successful.forEach(result => {
        console.log(`  • ${result.documentPath} (${result.documentsCount} chunks)`);
      });
    }

    if (failed.length > 0) {
      console.log("\nFailed to migrate:");
      failed.forEach(result => {
        console.log(`  • ${result.documentPath}: ${result.error}`);
      });
    }

    const totalChunks = successful.reduce((sum, r) => sum + (r.documentsCount || 0), 0);
    console.log(`\nTotal document chunks migrated: ${totalChunks}`);
  }

  private async cleanupParsedFiles(): Promise<void> {
    try {
      await rm(this.parsedDir, { recursive: true, force: true });
      console.log(`Removed directory: ${this.parsedDir}`);
    } catch (error) {
      console.error("Warning: Could not remove parsed files directory:", error);
      console.log("You may need to manually remove the /documents/parsed directory");
    }
  }
}

// Run migration if this file is executed directly
if (require.main === module) {
  const migration = new PostgreSQLMigrationScript();
  migration.run().catch(console.error);
}

export { PostgreSQLMigrationScript };
