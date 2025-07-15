# ResAI Research System Architecture

## System Architecture Diagram

```mermaid
graph TB
    %% User Interface Layer
    subgraph Frontend["Frontend (Next.js)"]
        subgraph ResearchPage["Research Page Component"]
            UI["Research Chat Page<br/>page.tsx"]
            MDS["Multi-Document Selector<br/>Component"]
            Chat["Chat Interface<br/>Component"]
            Preview["PDF Preview<br/>Component"]
        end

        subgraph UIComponents["UI Components"]
            Badge["Document Source Badge"]
            MarkdownComp["Markdown Renderer"]
            Loader["Loading Components"]
            Tabs["Tabs Component"]
        end
    end

    %% API Layer
    subgraph APIRoutes["API Routes (/api/research)"]
        DocAPI["/documents<br/>GET - List Documents"]
        PrepAPI["/prepare-index<br/>POST - Prepare Document Index"]
        ChatAPI["/chat<br/>POST - Chat with Documents"]
        FileAPI["/files/[...path]<br/>GET - Serve PDF Files"]
    end

    %% Service Layer
    subgraph Services["Document Processing Services"]
        DIM["Document Index Manager<br/>Singleton Service"]
        MDQE["Multi-Document Query Engine<br/>Singleton Service"]

        subgraph LlamaIndex["LlamaIndex Integration"]
            LP["LlamaParse Reader<br/>PDF Processing"]
            VSI["Vector Store Index<br/>Document Embeddings"]
            Retriever["Document Retriever<br/>Similarity Search"]
        end
    end

    %% External Services
    subgraph ExternalAPIs["External APIs"]
        DeepSeek["DeepSeek LLM<br/>Chat Completion"]
        HuggingFace["HuggingFace<br/>Text Embeddings"]
        LlamaCloud["LlamaCloud<br/>Document Parsing"]
    end

    %% Data Storage
    subgraph FileSystem["File System"]
        Docs["/documents/<br/>PDF Files"]
        Parsed["/documents/parsed/<br/>Cached Documents"]
        Cache["In-Memory Index Cache<br/>Map<string, VectorStoreIndex>"]
    end

    %% User Flow
    User((User)) --> UI
    UI --> MDS
    UI --> Chat
    UI --> Preview

    %% Document Selection Flow
    MDS -->|"Load Documents"| DocAPI
    DocAPI --> Docs
    MDS -->|"Select Documents"| PrepAPI
    PrepAPI --> DIM

    %% Document Processing Flow
    DIM --> LP
    LP --> LlamaCloud
    LP --> Parsed
    DIM --> VSI
    VSI --> HuggingFace
    VSI --> Cache

    %% Chat Flow
    Chat -->|"Send Query"| ChatAPI
    ChatAPI --> MDQE
    MDQE --> DIM
    MDQE --> Retriever
    Retriever --> VSI
    ChatAPI --> DeepSeek

    %% PDF Preview Flow
    Preview -->|"Display PDF"| FileAPI
    FileAPI --> Docs

    %% Styling
    classDef frontend fill:#e1f5fe,stroke:#0277bd,stroke-width:2px
    classDef api fill:#f3e5f5,stroke:#7b1fa2,stroke-width:2px
    classDef service fill:#e8f5e8,stroke:#2e7d32,stroke-width:2px
    classDef external fill:#fff3e0,stroke:#ef6c00,stroke-width:2px
    classDef storage fill:#fce4ec,stroke:#c2185b,stroke-width:2px

    class UI,MDS,Chat,Preview,Badge,MarkdownComp,Loader,Tabs frontend
    class DocAPI,PrepAPI,ChatAPI,FileAPI api
    class DIM,MDQE,LP,VSI,Retriever service
    class DeepSeek,HuggingFace,LlamaCloud external
    class Docs,Parsed,Cache storage
```

## Data Flow

### Document Selection Flow

1. User opens research page
2. System loads available documents from `/documents/` directory
3. User selects one or more documents
4. System prepares indices for selected documents
5. Documents are parsed using LlamaCloud (if not cached)
6. Vector embeddings are generated using HuggingFace
7. Indices are stored in memory cache

### Chat Query Flow

1. User enters a question
2. System retrieves relevant chunks from combined index
3. Context is organized by source document
4. Enhanced prompt is created with context and query
5. DeepSeek LLM generates streaming response
6. Response is displayed with source attribution
