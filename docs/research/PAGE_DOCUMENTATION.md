# ResAI - ResilientDB AI-Powered Resource Hub

## Project Overview

ResAI is a Next.js-based AI-powered research assistant specifically designed for Apache ResilientDB blockchain technology. The application combines advanced document parsing capabilities with AI chat functionality to provide researchers, students, and practitioners with an intelligent resource hub for understanding complex technical concepts related to ResilientDB, blockchain systems, and distributed computing.

# Research Page

## Purpose

`ResearchChatPage` is a React functional component built with Next.js and TailwindCSS. It provides an interactive research assistant where users can select academic documents (PDFs), ask questions about them, and receive streaming AI-generated responses, including inline source attribution.

---

## How It Works – Step-by-Step Flow

### 1. **Component Load (Mount Phase)**

- Triggers `useEffect` to fetch a list of available documents via `GET /api/research/documents`
- Applies `getDisplayTitle` to improve readability of document names

### 2. **User Selects Documents**

- Documents are selected via `MultiDocumentSelector`
- Selected documents are stored in `selectedDocuments`
- Triggers document preparation phase

### 3. **Indexing Phase** (Auto-triggered on selection)

- Sends `POST /api/research/prepare-index` with selected document paths
- Displays assistant message indicating indexing status
- On success: shows "ready" message so the user knows they can ask questions

### 4. **User Sends Question**

- Input value is validated (non-empty, documents selected, not loading)
- Appends user message + streaming placeholder for AI response
- Sends `POST /api/research/chat`
  - Payload: `{ query, documentPaths }`
  - Response: streamed text via a `ReadableStream`
  - Handles `__SOURCE_INFO__{...}` preamble (if included) for source attribution

- Updates the chat interface with streaming markdown + citation links

### 5. **Chat UI Updates**

- Scrolls to bottom automatically
- Displays messages in styled bubbles
- Assistant responses can contain:
  - Markdown content
  - Source documents (shown with `SourceAttribution` component)

---

## Key Functional Components

### State Variables

| Variable                        | Purpose                                |
| ------------------------------- | -------------------------------------- |
| `documents`                     | List of all documents available        |
| `selectedDocuments`             | User-selected documents for chat       |
| `messages`                      | Chat message history                   |
| `isLoading`, `isPreparingIndex` | Loading flags                          |
| `isSidebarCollapsed`            | Collapse state of the document sidebar |

### Functions

- `getDisplayTitle(filename)`: Converts filename into human-readable title
- `handleSendMessage()`: Manages chat submission, streaming updates, error handling
- `prepareDocumentIndex()`: Auto-triggers when selected documents change
- `handleDocumentSelect()` and `handleDocumentDeselect()`:
  Manage document selection and deselection

### Event Handlers

- Keyboard support: `handleKeyDown()` triggers message send on Enter
- Sidebar toggling: Chevron buttons show/hide the document list

---

## UI Layout Overview

### Sidebar (Document Selection)

- Renders on desktop as collapsible pane
- On mobile: shown using `Sheet` modal
- Built with `MultiDocumentSelector`

### Chat Window

- Messages render in aligned card components
- Input at bottom with Enter-to-send behavior
- Source citations appear under assistant messages

### PDF Preview Panel (Right)

- Renders `<iframe>`s of selected documents
- Tabs used to switch across multiple document previews
- Responsive: hidden on small screens

---

## API Overview

| Endpoint                      | Method | Purpose                                     |
| ----------------------------- | ------ | ------------------------------------------- |
| `/api/research/documents`     | GET    | Load available documents                    |
| `/api/research/prepare-index` | POST   | Index selected documents                    |
| `/api/research/chat`          | POST   | Ask questions and receive streaming answers |
| `/api/research/files/:path`   | GET    | Render PDF preview via iframe               |
