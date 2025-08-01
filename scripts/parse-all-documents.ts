import chalk from "chalk";
import { readdirSync, statSync } from "fs";
import { join } from "path";
import { configureLlamaSettings } from "../src/lib/config/llama-settings";
import { documentService } from "../src/lib/document-service";

/**
 * Script to parse and index all documents in the documents folder
 * This script will discover all PDF files and process them using the DocumentService
 */
async function parseAllDocuments() {
  console.log(chalk.bold.blue("📚 Parsing All Documents in Documents Folder"));
  console.log(chalk.gray("=".repeat(60)));

  try {
    // Step 1: Configure LlamaIndex settings
    console.log(chalk.blue("1. Configuring LlamaIndex settings..."));
    configureLlamaSettings();
    console.log(chalk.green("✅ LlamaIndex configured with HuggingFace embeddings"));

    // Step 2: Discover documents in the documents folder
    console.log(chalk.blue("\n2. Discovering documents..."));
    const documentsDir = join(process.cwd(), "documents");
    const documentPaths = await discoverDocuments(documentsDir);
    
    if (documentPaths.length === 0) {
      console.log(chalk.yellow("⚠️ No documents found in the documents folder"));
      return;
    }

    console.log(chalk.green(`✅ Found ${documentPaths.length} documents:`));
    documentPaths.forEach((path, index) => {
      const fileName = path.split("/").pop();
      console.log(chalk.gray(`  ${index + 1}. ${fileName}`));
    });

    // Step 3: Check if documents are already indexed
    console.log(chalk.blue("\n3. Checking if documents are already indexed..."));
    const alreadyIndexed = await documentService.areDocumentsIndexed(documentPaths);
    
    if (alreadyIndexed) {
      console.log(chalk.yellow("⚠️ Documents appear to be already indexed"));
      console.log(chalk.blue("Proceeding with re-indexing to ensure fresh data..."));
    } else {
      console.log(chalk.green("✅ Documents not yet indexed, proceeding with parsing"));
    }

    // Step 4: Parse and index all documents
    console.log(chalk.blue("\n4. Parsing and indexing documents..."));
    console.log(chalk.yellow("This may take several minutes depending on document size..."));
    
    const indexingStartTime = Date.now();
    
    try {
      const index = await documentService.indexDocuments(documentPaths);
      const indexingTime = Date.now() - indexingStartTime;
      
      console.log(chalk.green(`✅ Successfully indexed all documents in ${(indexingTime / 1000).toFixed(2)} seconds`));
      
    } catch (indexingError) {
      console.error(chalk.red("❌ Indexing failed:"), indexingError);
      
      // Try to index documents one by one to identify problematic documents
      console.log(chalk.yellow("\n🔍 Attempting to index documents individually..."));
      const successfulDocs: string[] = [];
      const failedDocs: string[] = [];
      
      for (const docPath of documentPaths) {
        try {
          console.log(chalk.blue(`  Processing: ${docPath.split("/").pop()}`));
          await documentService.indexDocuments([docPath]);
          successfulDocs.push(docPath);
          console.log(chalk.green(`    ✅ Success`));
        } catch (error) {
          failedDocs.push(docPath);
          console.log(chalk.red(`    ❌ Failed: ${error instanceof Error ? error.message : String(error)}`));
        }
      }
      
      console.log(chalk.blue(`\n📊 Individual indexing results:`));
      console.log(chalk.green(`  Successful: ${successfulDocs.length}/${documentPaths.length}`));
      console.log(chalk.red(`  Failed: ${failedDocs.length}/${documentPaths.length}`));
      
      if (failedDocs.length > 0) {
        console.log(chalk.red(`  Failed documents:`));
        failedDocs.forEach(doc => console.log(chalk.gray(`    - ${doc.split("/").pop()}`)));
      }
    }

    // Step 5: Test queries on indexed documents
    console.log(chalk.blue("\n5. Testing queries on indexed documents..."));
    
    const testQueries = [
      "What is ResilientDB?",
      "How does blockchain consensus work?",
      "What are the performance characteristics?",
      "What security features are mentioned?",
      "How does transaction processing work?"
    ];

    for (const query of testQueries) {
      console.log(chalk.yellow(`\nQuery: "${query}"`));
      
      try {
        const queryStartTime = Date.now();
        const result = await documentService.queryDocuments(query, {
          topK: 3,
          documentPaths: documentPaths
        });
        const queryTime = Date.now() - queryStartTime;
        
        console.log(chalk.cyan(`  └─ Found ${result.totalChunks} relevant chunks in ${queryTime}ms`));
        
        if (result.chunks.length > 0) {
          const topChunk = result.chunks[0];
          const preview = topChunk.content.substring(0, 120).replace(/\n/g, " ");
          const score = topChunk.metadata?.score?.toFixed(4) || 'N/A';
          const source = topChunk.source.split("/").pop() || 'Unknown';
          
          console.log(chalk.gray(`  └─ Best match (${score}): "${preview}..."`));
          console.log(chalk.gray(`  └─ Source: ${source}`));
        }
        
      } catch (queryError) {
        console.log(chalk.red(`  └─ Query failed: ${queryError instanceof Error ? queryError.message : String(queryError)}`));
      }
    }

    // Step 6: Display final statistics
    console.log(chalk.blue("\n6. Document processing statistics..."));
    
    try {
      const cacheStats = await documentService.getCacheStats();
      console.log(chalk.cyan("Document Cache Statistics:"));
      console.log(chalk.gray(`  Documents in cache: ${cacheStats.documentCount}`));
      console.log(chalk.gray(`  Total chunks: ${cacheStats.totalChunks}`));
      console.log(chalk.gray(`  Oldest document: ${cacheStats.oldestDocument || 'None'}`));
      console.log(chalk.gray(`  Newest document: ${cacheStats.newestDocument || 'None'}`));
    } catch (statsError) {
      console.log(chalk.yellow(`⚠️ Could not retrieve cache statistics: ${statsError}`));
    }

    // Step 7: Provide usage information
    console.log(chalk.blue("\n7. Usage information..."));
    console.log(chalk.cyan("Your documents are now indexed and ready for querying!"));
    console.log(chalk.gray("You can now use the chat interface or API to query these documents:"));
    documentPaths.forEach(path => {
      const fileName = path.split("/").pop();
      const displayName = getDocumentDisplayName(fileName || "");
      console.log(chalk.gray(`  - ${displayName} (${fileName})`));
    });

    console.log(chalk.bold.green("\n🎉 Document parsing completed successfully!"));
    console.log(chalk.gray("=".repeat(60)));

  } catch (error) {
    console.error(chalk.red("\n❌ Document parsing failed:"));
    console.error(chalk.red(error instanceof Error ? error.message : String(error)));
    if (error instanceof Error && error.stack) {
      console.error(chalk.gray(error.stack));
    }
    process.exit(1);
  }
}

/**
 * Discover all PDF documents in the specified directory
 */
async function discoverDocuments(documentsDir: string): Promise<string[]> {
  try {
    const files = readdirSync(documentsDir);
    const documentPaths: string[] = [];
    
    for (const file of files) {
      const fullPath = join(documentsDir, file);
      const stats = statSync(fullPath);
      
      if (stats.isFile() && file.toLowerCase().endsWith('.pdf')) {
        // Convert to relative path from project root
        const relativePath = `documents/${file}`;
        documentPaths.push(relativePath);
      }
    }
    
    return documentPaths.sort(); // Sort alphabetically
    
  } catch (error) {
    console.error(chalk.red(`Failed to discover documents in ${documentsDir}:`), error);
    return [];
  }
}

/**
 * Get a human-readable display name for a document
 */
function getDocumentDisplayName(fileName: string): string {
  // Remove .pdf extension and replace hyphens/underscores with spaces
  return fileName
    .replace(/\.pdf$/i, '')
    .replace(/[-_]/g, ' ')
    .replace(/\b\w/g, (char) => char.toUpperCase()); // Title case
}

/**
 * Parse specific documents by providing file names
 */
export async function parseSpecificDocuments(fileNames: string[]): Promise<void> {
  console.log(chalk.bold.blue(`📄 Parsing Specific Documents: ${fileNames.join(', ')}`));
  
  const documentPaths = fileNames.map(fileName => `documents/${fileName}`);
  
  // Verify all files exist
  const documentsDir = join(process.cwd(), "documents");
  for (const fileName of fileNames) {
    const fullPath = join(documentsDir, fileName);
    try {
      statSync(fullPath);
    } catch (error) {
      throw new Error(`Document not found: ${fileName}`);
    }
  }
  
  // Configure settings and index
  configureLlamaSettings();
  
  const startTime = Date.now();
  await documentService.indexDocuments(documentPaths);
  const endTime = Date.now();
  
  console.log(chalk.green(`✅ Successfully parsed ${fileNames.length} documents in ${(endTime - startTime) / 1000}s`));
}

// Run the script if this file is executed directly
if (require.main === module) {
  parseAllDocuments().catch(console.error);
}

export { discoverDocuments, parseAllDocuments };

