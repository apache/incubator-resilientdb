#!/usr/bin/env tsx

import chalk from "chalk";
import { documentService } from "../src/lib/document-service";

async function main() {
  try {
    console.log(chalk.blue("🔍 Analyzing duplicate documents in vector store..."));
    
    // Get duplicate statistics
    const stats = await documentService.getDuplicateStats();
    
    console.log(chalk.cyan("\n📊 Duplicate Analysis Results:"));
    console.log(chalk.gray(`  Total documents: ${stats.totalDocuments}`));
    console.log(chalk.gray(`  Unique documents: ${stats.uniqueDocuments}`));
    console.log(chalk.yellow(`  Duplicate documents: ${stats.duplicateDocuments}`));
    
    if (stats.duplicateDocuments === 0) {
      console.log(chalk.green("\n✅ No duplicates found! Your vector store is clean."));
      return;
    }
    
    console.log(chalk.yellow(`\n🔍 Top duplicate groups:`));
    stats.documentGroups.forEach((group, index) => {
      console.log(chalk.gray(`  ${index + 1}. ${group.source_document}: ${group.count} copies`));
      console.log(chalk.gray(`     Preview: ${group.content_preview}...`));
    });
    
    // Ask for confirmation (in a real scenario, you might want to add readline for user input)
    console.log(chalk.blue("\n🧹 Removing duplicate documents..."));
    
    await documentService.removeDuplicateDocuments();
    
    // Get updated statistics
    const updatedStats = await documentService.getDuplicateStats();
    console.log(chalk.green(`\n✅ Cleanup complete!`));
    console.log(chalk.gray(`  Documents remaining: ${updatedStats.totalDocuments}`));
    console.log(chalk.gray(`  Duplicates removed: ${stats.duplicateDocuments - updatedStats.duplicateDocuments}`));
    
  } catch (error) {
    console.error(chalk.red("❌ Error during cleanup:"), error);
    
    if (error instanceof Error && error.message.includes('does not exist')) {
      console.log(chalk.yellow("\n💡 It looks like the vector store table doesn't exist yet."));
      console.log(chalk.gray("   This might be because:"));
      console.log(chalk.gray("   1. The vector store hasn't been set up yet"));
      console.log(chalk.gray("   2. No documents have been indexed yet"));
      console.log(chalk.gray("   3. The table name configuration is incorrect"));
      console.log(chalk.blue("\n🔧 To diagnose the issue, run:"));
      console.log(chalk.green("   npm run diagnose:database"));
      console.log(chalk.blue("\n🔧 To set up the vector store, run:"));
      console.log(chalk.green("   npm run db:setup-vector"));
    }
    
    process.exit(1);
  }
}

if (require.main === module) {
  main().catch(console.error);
}