**Title:** GraphQ-LLM: Building an AI Query Tutor for ResilientDB

**Authors:** Aayusha Hadke, Celine John Philip, Sandhya Ghanathe, Sophie Quynn, Theodore Pan

**Project:** ECS 265 DDS Mid-term Progress Report

**Tags:** #ResilientDB, #GraphQL, #AI, #LLM, #Blockchain

# GraphQ-LLM: AI-Powered GraphQL Query Tutor for ResilientDB

## Introduction

GraphQ-LLM is an intelligent AI assistant that helps developers learn, understand, and optimize GraphQL queries for ResilientDB. Built with Retrieval-Augmented Generation (RAG) and integrated with the ResilientApp ecosystem, it provides real-time explanations, optimization suggestions, and performance insights for your GraphQL queries.

---

## 🎯 What is GraphQ-LLM?

GraphQ-LLM is a comprehensive AI tutor that transforms how developers interact with GraphQL APIs. Instead of searching through documentation or trial-and-error query writing, developers can ask questions in natural language or paste their queries to get:

- **Detailed Explanations**: Understand what each query does, how fields work, and what to expect in responses
- **Optimization Suggestions**: Get actionable recommendations to improve query performance and efficiency
- **Efficiency Metrics**: See estimated execution time, resource usage, and complexity scores
- **Documentation Context**: Access relevant ResilientDB and GraphQL documentation through semantic search

---

## 🏗️ How It Works

### Architecture Overview

GraphQ-LLM uses a three-tier architecture that combines AI, vector search, and real-time monitoring:

```
┌─────────────────┐
│   Nexus UI      │  (Next.js Frontend)
│ GraphQL Tutor   │
└────────┬────────┘
         │
         ▼
┌─────────────────┐
│ GraphQ-LLM      │  (HTTP API Backend)
│ Backend         │  • Explanation Service
│                 │  • Optimization Service
│                 │  • Efficiency Estimator
└────────┬────────┘
         │
    ┌────┴────┐
    ▼         ▼
┌────────┐  ┌─────────────┐
│ResDB   │  │ ResLens     │
│GraphQL │  │ Monitoring  │
│Server  │  │ API         │
└────────┘  └─────────────┘
```

### Core Components

1. **RAG System (Retrieval-Augmented Generation)**
   - Stores GraphQL documentation in ResilientDB with vector embeddings
   - Uses semantic search to find relevant documentation chunks
   - Combines retrieved context with LLM for accurate, contextual responses

2. **AI Explanation Service**
   - Detects whether input is a GraphQL query or natural language question
   - For queries: Analyzes structure, fields, operations, and provides explanations
   - For questions: Retrieves relevant docs and generates comprehensive answers

3. **Optimization Engine**
   - Analyzes query structure and complexity
   - Compares with historical query patterns (via ResLens)
   - Provides specific, actionable optimization recommendations

4. **Efficiency Estimator**
   - Calculates query complexity scores
   - Estimates execution time and resource usage
   - Provides real-time metrics when ResLens is enabled

---

## 🚀 Key Features

### 1. **Intelligent Query Explanation**

Paste any GraphQL query and get a detailed breakdown:

```graphql
{
  getTransaction(id: "123") {
    id
    asset
    amount
  }
}
```

**Response includes:**
- What the query does (in plain English)
- How each field and operation works
- Expected response format
- Common use cases

### 2. **Natural Language Q&A**

Ask questions like:
- "How do I retrieve a transaction by ID in GraphQL?"
- "What is the difference between query and mutation?"
- "How can I optimize my GraphQL queries?"

The system retrieves relevant documentation and provides comprehensive answers with examples.

### 3. **Query Optimization Suggestions**

Get actionable recommendations:
- Remove unused fields
- Use field aliases for clarity
- Add filters to reduce result size
- Optimize nested queries

### 4. **Performance Metrics**

See efficiency scores, estimated execution times, and resource usage to understand query performance at a glance.

---

## 📦 Setup Overview

GraphQ-LLM is fully dockerized for easy deployment. Here's what you need:

### Prerequisites

- **Docker & Docker Compose** - For running all services
- **Node.js 18+** - For local development (optional)
- **Gemini API Key** - For LLM capabilities (get from [Google AI Studio](https://makersuite.google.com/app/apikey))
- **Nexus Repository** - Separate Next.js frontend (cloned from [ResilientApp/nexus](https://github.com/ResilientApp/nexus))

### Quick Start

1. **Clone and Install**
   ```bash
   git clone <graphq-llm-repo>
   cd graphq-llm
   npm install
   ```

2. **Configure Environment**
   ```bash
   # Create .env file and add your Gemini API key
   LLM_PROVIDER=gemini
   LLM_API_KEY=your_gemini_api_key_here
   RESILIENTDB_GRAPHQL_URL=http://localhost:5001/graphql
   ```

3. **Start Services with Docker**
   ```bash
   # Start ResilientDB (database + GraphQL server)
   docker-compose -f docker-compose.dev.yml up -d resilientdb
   
   # Start GraphQ-LLM Backend
   docker-compose -f docker-compose.dev.yml up -d graphq-llm-backend
   
   # (Optional) Start ResLens for performance monitoring
   docker-compose -f docker-compose.dev.yml up -d reslens-middleware reslens-frontend
   ```

4. **Ingest Documentation**
   ```bash
   npm run ingest:graphql
   # This loads all GraphQL docs into ResilientDB for RAG
   ```

5. **Set Up Nexus Frontend**
   - Clone Nexus repository separately
   - Add GraphQ-LLM integration files (see `NEXUS_UI_EXTENSION_GUIDE.md`)
   - Start with `npm run dev`

6. **Access the Tool**
   - Open `http://localhost:3000/graphql-tutor` in your browser
   - Start querying or asking questions!

### Service Architecture

All services run in Docker containers:

- **ResilientDB** (Port 18000, 5001) - Database with GraphQL server
- **GraphQ-LLM Backend** (Port 3001) - HTTP API for Nexus integration
- **GraphQ-LLM MCP Server** - For MCP client integration (stdio transport)
- **ResLens Middleware** (Port 3003) - Performance monitoring API (optional)
- **ResLens Frontend** (Port 5173) - Performance monitoring UI (optional)

---

## 💡 How It Helps Developers

### Learning GraphQL

New to GraphQL? GraphQ-LLM explains:
- Query syntax and structure
- Field selection and arguments
- Mutations vs queries
- Schema exploration
- Best practices

### Query Optimization

Working on performance? Get:
- Complexity analysis
- Field selection recommendations
- Execution time estimates
- Resource usage insights
- Historical query comparisons (with ResLens)

### Troubleshooting

Stuck on an error? The system helps:
- Understand query structure issues
- Find relevant documentation
- Compare with similar working queries
- Get optimization suggestions

---

## 🔧 Technology Stack

- **Backend**: Node.js/TypeScript with RAG architecture
- **LLM**: Gemini 2.5 Flash Lite (configurable: DeepSeek, OpenAI, Anthropic, Hugging Face, local models)
- **Embeddings**: Local (Xenova/all-MiniLM-L6-v2) or Hugging Face API
- **Database**: ResilientDB for vector storage and document chunks
- **Frontend**: Next.js (Nexus integration)
- **Monitoring**: ResLens for real-time performance metrics
- **Protocol**: MCP (Model Context Protocol) for secure AI tool integration

---

## 🌐 Integration with ResilientApp Ecosystem

GraphQ-LLM is designed to work seamlessly with:

- **ResilientDB**: The underlying blockchain database
- **Nexus**: The ResilientApp frontend platform
- **ResLens**: Performance monitoring and profiling tools

Together, these tools provide a complete development and monitoring experience for ResilientDB applications.

---

## 📚 Documentation & Resources

Complete setup instructions are available in:
- **TEAM_SETUP.md** - Step-by-step setup guide
- **TEST_DOCKER_SERVICES.md** - Service verification guide
- **QUERY_TUTOR_EXAMPLES.md** - Example queries and questions
- **NEXUS_UI_EXTENSION_GUIDE.md** - Frontend integration guide

---

## 🎓 Example Use Cases

### Scenario 1: Learning GraphQL

**Input:**
```
"What is GraphQL and how do I write a basic query?"
```

**Output:**
Comprehensive explanation with examples, documentation references, and links to relevant guides.

### Scenario 2: Explaining a Query

**Input:**
```graphql
{
  getTransaction(id: "abc123") {
    id
    asset
    amount
  }
}
```

**Output:**
- Detailed breakdown of what this query does
- Explanation of each field
- Expected response format
- Use cases and examples

### Scenario 3: Optimizing Performance

**Input:**
```graphql
{
  getTransaction(id: "test") {
    id version amount uri type publicKey signerPublicKey operation metadata asset
  }
}
```

**Output:**
- Efficiency score: 75/100
- Optimization suggestions: "Consider selecting only needed fields"
- Estimated execution time: 45ms
- Complexity: Medium

---

## 🚦 Getting Started Checklist

- [ ] Install Docker and Docker Compose
- [ ] Clone GraphQ-LLM repository
- [ ] Get Gemini API key from Google AI Studio
- [ ] Configure `.env` file with API key
- [ ] Start ResilientDB container
- [ ] Start GraphQ-LLM Backend container
- [ ] Ingest documentation (one-time setup)
- [ ] Clone Nexus repository
- [ ] Add GraphQ-LLM integration to Nexus
- [ ] Start Nexus frontend
- [ ] Access `http://localhost:3000/graphql-tutor`
- [ ] Start querying!

---

## 🎯 Conclusion

GraphQ-LLM bridges the gap between complex GraphQL documentation and practical query writing. By combining AI-powered explanations, semantic search, and performance monitoring, it provides developers with an intelligent assistant that makes learning and optimizing GraphQL queries effortless.

Whether you're a GraphQL beginner or an experienced developer looking to optimize queries, GraphQ-LLM offers the insights and recommendations you need to write better, faster, and more efficient queries for ResilientDB.

**Ready to get started?** Follow the complete setup guide in `TEAM_SETUP.md` and begin exploring the power of AI-assisted GraphQL development!

---

*For detailed setup instructions, troubleshooting, and advanced configuration, see the complete documentation in the repository.*

You can follow our progress on [GitHub here](#).
