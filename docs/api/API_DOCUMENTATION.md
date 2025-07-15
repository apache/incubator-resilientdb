# API Documentation: /api/chat

This API endpoint provides a chat interface for general-purpose AI conversations. It is designed to be used by the main chat UI in the application.

## Overview

- **Path:** `/api/chat`
- **Methods:** `POST`, `GET`
- **Purpose:** Handles chat requests and streams AI-generated responses using the chat engine service.

## How It Works

### Initialization

- On first use, the chat engine service is initialized (singleton pattern).
- If initialization fails, an error is returned.

### POST Request

- **Input:** `{ query: string }`
- **Process:**
  1. Validates that a `query` is provided.
  2. Ensures the chat engine service is initialized.
  3. Checks the service status.
  4. Sends the query to the chat engine and streams the response.
- **Output:**
  - Streams the AI response as plain text.
  - Returns error details if any step fails.

### GET Request

- **Purpose:** Health check for the chat engine service.
- **Output:**
  - Returns a JSON object with the service status and a message.

## Key Concepts

- **Streaming:** Responses are streamed for real-time chat experience.
- **Service Status:** The endpoint checks and reports the status of the underlying chat engine.

---

# API Documentation: /api/research/chat

This endpoint enables document-aware AI chat, allowing users to ask questions about one or more research documents.

## Overview

- **Path:** `/api/research/chat`
- **Methods:** `POST`
- **Purpose:** Answers questions about selected documents using LLMs and vector search.

## How It Works

### POST Request

- **Input:** `{ query: string, documentPath?: string, documentPaths?: string[] }`
- **Process:**
  1. Validates input (query and document(s) required).
  2. Configures LLM and embedding models.
  3. Retrieves or builds vector indices for the selected documents.
  4. Retrieves relevant context from the documents using vector search.
  5. Constructs a prompt with context and streams the LLM's answer.
- **Output:**
  - Streams the answer, including source information for attribution.
  - Returns error details if any step fails.

## Key Concepts

- **Multi-Document Support:** Can answer questions about multiple documents at once.
- **Context Retrieval:** Uses vector search to find relevant passages.
- **Source Attribution:** Streams source info to the client for transparency.

---

# API Documentation: /api/research/documents

This endpoint lists available research documents for selection in the UI.

## Overview

- **Path:** `/api/research/documents`
- **Methods:** `GET`
- **Purpose:** Returns a list of available documents (PDF, DOC, PPT, etc.).

## How It Works

### GET Request

- **Process:**
  1. Reads the `documents/` directory.
  2. Filters for allowed file types.
  3. Returns metadata for each document (name, path, size, upload date).
- **Output:**
  - JSON array of document metadata.

## Key Concepts

- **File Filtering:** Only includes supported document types.
- **Metadata:** Provides file info for display and selection.

---

# Library: document-index-manager.ts

This module manages the parsing, indexing, and retrieval of research documents for AI-powered Q&A.

## Overview

- **Purpose:** Handles document parsing, caching, vector index creation, and retrieval for both single and multiple documents.
- **Singleton Pattern:** Ensures a single instance manages all indices in memory.

## How It Works

### Index Preparation

- Checks if parsed/cached files exist and are up-to-date.
- If not, parses the document and saves the result.
- Builds a vector index for fast semantic search.

### Index Retrieval

- Retrieves an in-memory or cached index for a document.
- Supports rebuilding from parsed files if needed.

### Multi-Document Support

- Prepares and combines indices for multiple documents.
- Allows retrieval of context across several documents for multi-document Q&A.

### Utility Methods

- List, clear, and manage indices in memory.

## Key Concepts

- **Vector Search:** Enables semantic retrieval of relevant passages.
- **Caching:** Avoids redundant parsing and speeds up repeated queries.
- **Source Attribution:** Maintains document source info for each passage.

---
