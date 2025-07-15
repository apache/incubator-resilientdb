# Simple PDF Chat Setup Guide

This guide will help you set up the simplified PDF chat functionality that allows you to select PDFs, preview them, and chat with their content using LlamaParse.

## Prerequisites

### 1. API Keys Required

You need the following API keys:

#### DeepSeek API Key

- Go to [DeepSeek Platform](https://platform.deepseek.com/)
- Sign up and create an API key
- This is used for the AI chat responses

#### LlamaCloud API Key

- Go to [LlamaCloud](https://cloud.llamaindex.ai)
- Sign up and get your API key
- This is used for document parsing with LlamaParse

### 2. Environment Setup

Create a `.env.local` file in your project root:

```env
# Required: DeepSeek API Key for the LLM
DEEPSEEK_API_KEY=your_deepseek_api_key_here

# Required: LlamaCloud API Key for document parsing
LLAMA_CLOUD_API_KEY=your_llamacloud_api_key_here

# Optional: DeepSeek model (defaults to "deepseek-chat")
DEEPSEEK_MODEL=deepseek-chat
```

## Features

### Current Implementation

1. **Document Selection**: Choose from PDFs in the `/documents` folder using a dropdown
2. **PDF Preview**: Native browser PDF viewer on the right side
3. **Simple Chat**: Ask questions about the selected PDF
4. **Text Extraction**: Uses LlamaParse for reliable text extraction from PDFs
5. **In-Memory Caching**: Parsed documents are cached in memory to avoid re-parsing

### What's Removed

- Layout extraction and highlighting
- Complex file-based caching
- Source citations with visual mapping
- Multiple API endpoints
- Image serving
- Complex UI components

## Usage

1. **Access the Interface**:
   - Navigate to `/placeholder` in your app
   - You'll see a two-panel layout: chat on left, PDF preview on right

2. **Add Documents**:
   - Place PDF files in the `/documents` folder
   - They will automatically appear in the dropdown

3. **Start Chatting**:
   - Select a document from the dropdown
   - The PDF will load in the preview panel
   - Start asking questions about the content

## API Endpoints

### `/api/research/documents`

- **Method**: GET
- **Description**: Lists available PDF documents
- **Returns**: Array of document metadata

### `/api/research/chat`

- **Method**: POST
- **Body**: `{ query: "your question", documentPath: "documents/filename.pdf" }`
- **Description**: Chat with a specific document using LlamaParse + DeepSeek
- **Returns**: Streaming text response

## How It Works

1. **Document Loading**: When you select a PDF, it loads in the browser's native PDF viewer
2. **First Question**: When you ask your first question about a document, LlamaParse extracts the text
3. **Vector Index**: The extracted text is converted to a vector index for semantic search
4. **Chat Response**: DeepSeek generates responses based on relevant document content
5. **Caching**: The vector index is cached in memory to avoid re-parsing the same document

## Troubleshooting

### "Failed to parse document" Error

**Most Common Causes:**

1. **Invalid LlamaCloud API Key**: Check your API key at [LlamaCloud](https://cloud.llamaindex.ai)
2. **Insufficient Credits**: Check your LlamaCloud account balance
3. **Unsupported PDF**: Some PDFs may not be parseable

### Documents Not Appearing

- Check that PDF files are in the `/documents` folder
- Ensure files have `.pdf` extension
- Check browser console for errors

### Chat Not Working

- Verify `DEEPSEEK_API_KEY` is set correctly
- Ensure the document was successfully parsed
- Check browser network tab for API errors

### PDF Not Loading

- Ensure the PDF file is accessible
- Check browser console for loading errors
- Try a different PDF to isolate the issue

## Development Notes

### Performance

- Documents are parsed on first use (lazy loading)
- Vector indices are cached in memory during the session
- Large PDFs may take longer to parse initially

### Limitations

- No persistent caching (re-parses on server restart)
- Single document chat only (no multi-document queries)
- No visual highlighting or source citations
- Relies on browser's built-in PDF viewer

### Future Enhancements

If needed, you could add back:

- File-based caching for persistence
- Multi-document chat capabilities
- Source citations (without visual highlighting)
- Document upload functionality

## File Structure

```
src/app/
├── placeholder/
│   └── page.tsx                 # Main chat interface
└── api/research/
    ├── documents/route.ts       # List available documents
    └── chat/route.ts           # Chat with documents

documents/                       # Place your PDFs here
├── paper1.pdf
├── paper2.pdf
└── ...
```

This simplified implementation focuses on the core functionality: select a PDF, preview it, and chat with its content using reliable LlamaParse text extraction.
