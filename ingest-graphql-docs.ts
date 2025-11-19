#!/usr/bin/env tsx

/**
 * Lightweight ingestion script focused on GraphQL docs.
 * Targets only graphql-official, graphql-spec, community guides, and schema docs,
 * so we can ingest faster without hitting Hugging Face rate limits on the full corpus.
 */

import * as fs from 'fs';
import * as path from 'path';
import { IngestionPipeline } from './src/rag/ingestion-pipeline';
import { DocumentLoader, LoadedDocument } from './src/rag/document-loader';

async function loadDirectoryIfExists(
  loader: DocumentLoader,
  dirPath: string
): Promise<LoadedDocument[]> {
  if (!fs.existsSync(dirPath)) {
    return [];
  }

  return loader.loadDirectory(dirPath, {
    recursive: true,
    extensions: ['.md', '.mdx', '.txt'],
    exclude: ['node_modules', '.git', 'dist', 'build'],
  });
}

async function loadFileIfExists(
  loader: DocumentLoader,
  filePath: string
): Promise<LoadedDocument[]> {
  if (!fs.existsSync(filePath)) {
    return [];
  }

  return [await loader.loadFile(filePath)];
}

async function ingestGraphQLDocs() {
  console.log('📚 GraphQL-only Ingestion');
  console.log('============================\n');

  // Ensure ResilientDB GraphQL endpoint is set (defaults to local dev port 5001)
  if (!process.env.RESILIENTDB_GRAPHQL_URL) {
    process.env.RESILIENTDB_GRAPHQL_URL = 'http://localhost:5001/graphql';
  }

  const loader = new DocumentLoader();
  const pipeline = new IngestionPipeline({
    maxChunkTokens: 400,
    chunkOverlapTokens: 40,
    batchSize: 5, // keep batches small to avoid HF rate limits
  });

  const projectRoot = process.cwd();
  const targetDirs = [
    path.join(projectRoot, 'docs', 'graphql-official'),
    path.join(projectRoot, 'docs', 'graphql-spec'),
    path.join(projectRoot, 'docs', 'community-guides'),
  ];

  const singleFiles = [
    path.join(projectRoot, 'docs', 'GRAPHQL_SETUP.md'),
    path.join(projectRoot, 'docs', 'GRAPHQL_SCHEMA_EXAMPLES.md'),
    path.join(projectRoot, 'docs', 'resilientdb-schema.md'),
  ];

  const documents: LoadedDocument[] = [];

  for (const dir of targetDirs) {
    const docs = await loadDirectoryIfExists(loader, dir);
    if (docs.length > 0) {
      console.log(`✅ Loaded ${docs.length} docs from ${dir}`);
      documents.push(...docs);
    }
  }

  for (const file of singleFiles) {
    const doc = await loadFileIfExists(loader, file);
    if (doc.length > 0) {
      console.log(`✅ Loaded file ${file}`);
      documents.push(...doc);
    }
  }

  if (documents.length === 0) {
    console.log('⚠️  No GraphQL-specific documents found. Exiting.');
    return;
  }

  console.log(`\n🚀 Ingesting ${documents.length} GraphQL-specific documents...\n`);

  let progressLogged = 0;
  const progress = await pipeline.ingestDocuments(documents, undefined, {
    onProgress: (current) => {
      const percentage = current.totalChunks
        ? Math.round((current.processedChunks / current.totalChunks) * 100)
        : 0;
      if (percentage >= progressLogged + 10 || current.processedChunks === current.totalChunks) {
        console.log(
          `📊 Progress: ${current.processedChunks}/${current.totalChunks} chunks (${percentage}%)`
        );
        progressLogged = percentage;
      }
    },
  });

  console.log('\n✅ GraphQL ingestion complete!');
  console.log(`Documents processed: ${progress.processedDocuments}/${progress.totalDocuments}`);
  console.log(`Chunks stored: ${progress.storedChunks}`);
  if (progress.errors.length > 0) {
    console.log('⚠️  Some errors occurred:');
    progress.errors.forEach((err, idx) => {
      console.log(`  ${idx + 1}. ${err.documentId}: ${err.error}`);
    });
  }
}

if (require.main === module) {
  ingestGraphQLDocs().catch((error) => {
    console.error('❌ Fatal error during GraphQL ingestion:', error);
    process.exit(1);
  });
}


