<!--
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
-->

## Nexus - Research Agent
<img width="1728" height="834" alt="Screenshot 2025-08-07 at 12 25 54 PM" src="https://github.com/user-attachments/assets/2be8d59d-f493-4c4f-abef-02eb76ab3af6" />

Nexus is a Next.js application that helps students and researchers work with Apache ResilientDB and related distributed systems literature. It provides grounded, citation-backed answers over selected documents and introduces agentic capabilities for research workflows.

The project is built on LlamaIndex for retrieval and workflow orchestration, DeepSeek for reasoning, Gemini embeddings for vectorization, and Postgres with pgvector for storage.

### What you can do today
- Chat over one or more documents with inline citations
- Ingest local PDFs to a pgvector index via LlamaParse
- Preview PDFs alongside the conversation
- Use a ResilientDB-focused agent with retrieval and web search tools
- Persist short/long‑term memory per session in Postgres

### Status
- Core RAG: implemented
- Multi-document ingestion + retrieval: implemented
- Agentic chat + memory: in progress


## Quickstart

### Prerequisites
- Node.js 18.18+ (Node 20+ recommended)
- Postgres 14+ with the pgvector extension
- Accounts/keys:
  - DeepSeek API key
  - Google Gemini API key
  - LlamaCloud API key (for LlamaParse JSON mode)
  - Tavily API key (optional; enables web search tool)

### 1) Create and configure your database
Ensure pgvector is available in your Postgres instance, then create a database (e.g., `nexus`) and enable the extension:

```sql
CREATE EXTENSION IF NOT EXISTS vector;
```

Tables for memory and document embeddings are created automatically on first run.

### 2) Add environment variables
Create a `.env` file in the project root:

```bash
DATABASE_URL=postgres://USER:PASSWORD@localhost:5432/nexus

# Must match the embedding model dimension. If you use Gemini Embedding 001, set 768.
# Adjust if you change the embedding model.
EMBEDDING_DIM=768

DEEPSEEK_API_KEY=your_deepseek_key
DEEPSEEK_MODEL=deepseek-chat

# Required for LlamaParse JSON mode used during ingestion
LLAMA_CLOUD_API_KEY=your_llamacloud_key
LLAMA_CLOUD_PROJECT_NAME=
LLAMA_CLOUD_INDEX_NAME=
LLAMA_CLOUD_BASE_URL=

# Optional but recommended for web search tool
TAVILY_API_KEY=

# Embeddings
GEMINI_API_KEY=your_gemini_key
```

Note: `EMBEDDING_DIM` must match your embedding model. With `Gemini Embedding 001` it is typically 768. If you switch models, update this value accordingly.

### 3) Install and run

```bash
npm install
npm run dev
```

Visit `http://localhost:3000/research`.

### 4) Add documents
Place your PDFs (and optionally DOC/DOCX/PPT/PPTX) in the `documents/` folder at the repo root. The app will list them in the sidebar. Select one or more, then Nexus will parse and index them on demand.


## How it works

### Architecture (high level)
- UI: Next.js App Router (`/research`) with a chat panel and a preview panel
- Retrieval and ingestion: LlamaIndex pipeline with SentenceSplitter, SummaryExtractor, and Gemini embeddings stored in Postgres pgvector
- Agent: LlamaIndex workflow with DeepSeek as the LLM, two main tools:
  - `search_documents`: queries the local vector index filtered by the selected documents
  - `search_web`: optional Tavily-backed web search
- Memory: session‑scoped short/long‑term memory persisted to pgvector
- Streaming: server streams model deltas; client renders responses and citations in real time

### Endpoints
- `GET /api/research/documents` — lists available files in `documents/`
- `GET /api/research/files/[...path]` — serves files from `documents/` for preview
- `POST /api/research/prepare-index` — parses and ingests selected files to the vector index (LlamaParse JSON mode)
- `POST /api/research/chat` — runs the ResilientDB agent with retrieval/memory and streams the response

### Session memory
Each chat session is identified by a `?session=<uuid>` query param. The agent attaches a memory that persists across turns for that session and stores long‑term facts in pgvector.

### Citations
Model responses use `[^id]` inline markers. The UI renders them as badges with source titles and page numbers when available.


## Using the app
1. Add PDFs to `documents/`.
2. Start the app and open `/research`.
3. Select documents in the sidebar. The app will prepare an index for the selection.
4. Ask questions. The agent retrieves from the selected documents first, and may use web search if needed.
5. Toggle Code Composer to draft code grounded in the selected papers (experimental).


## Configuration reference

Environment variables from `src/config/environment.ts`:

- `DATABASE_URL`: Postgres connection string
- `EMBEDDING_DIM`: embedding dimensionality used to initialize pgvector tables
- `DEEPSEEK_API_KEY`: required for DeepSeek LLM
- `DEEPSEEK_MODEL`: `deepseek-chat` or `deepseek-coder`
- `LLAMA_CLOUD_API_KEY`: required for LlamaParse JSON parsing during ingestion
- `LLAMA_CLOUD_PROJECT_NAME`, `LLAMA_CLOUD_INDEX_NAME`, `LLAMA_CLOUD_BASE_URL`: optional, reserved for future cloud indexing
- `TAVILY_API_KEY`: optional, enables the web search tool
- `GEMINI_API_KEY`: required for Gemini embeddings

If you change the embedding model, ensure `EMBEDDING_DIM` matches it and re‑create the vector tables if needed.


## Scripts
- `npm run dev` — start the Next.js dev server
- `npm run build` — build for production
- `npm run start` — start the production server
- `npm run lint` — lint the codebase


## Tech stack
- Next.js 15, React 19, TypeScript
- TailwindCSS, Radix UI primitives
- LlamaIndex (agents, ingestion pipeline, retriever)
- DeepSeek (LLM), Gemini embeddings
- Postgres + pgvector
- Tavily (optional web search)


## Notes
- This project is intended for educational and research use around Apache ResilientDB and related systems. Use appropriate discretion when interpreting generated code.

