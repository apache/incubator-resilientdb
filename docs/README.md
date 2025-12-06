---
**Title:** GraphQ-LLM: Building an AI Query Tutor for ResilientDB
**Authors:** Aayusha Hadke, Celine John Philip, Sandhya Ghanathe, Sophie Quynn, Theodore Pan
**Project:** ECS 265 DDS Mid-term Progress Report
**Tags:** #ResilientDB, #GraphQL, #AI, #LLM, #Blockchain
---

# GraphQ-LLM: An AI Query Tutor for ResilientDB

Developing blockchain applications can be daunting. While tools like **ResilientDB** offer robust infrastructure, the learning curve for writing efficient GraphQL queries remains a barrier for many developers. 

The existing AI tools, such as the Nexus model, are limited—they are "document-centric," meaning they can read documentation but can't see what is actually happening inside the database.

**Enter GraphQ-LLM.**

Our team is building an AI Query Tutor that goes beyond static documentation. By integrating a Large Language Model (LLM) directly with ResilientDB's execution metrics, we are building a tool that doesn't just tell you *how* to write a query, but predicts *how efficiently* it will run.

## The Problem: Data Silos
Currently, developers have to switch contexts constantly. They look at documentation in one window, write code in another, and check performance metrics in a third. 
* **Nexus (Existing):** Limited to document interaction.
* **The Goal:** Novel interactions with the inner workings of ResilientDB GraphQL.

## The Solution: The Model Context Protocol (MCP)
The core innovation of GraphQ-LLM is the **Model Context Protocol (MCP) server**. 

We are designing this server to securely and bi-directionally connect ResilientDB data sources with AI tools. This breaks down the barriers that silo data. instead of a generic AI, GraphQ-LLM will utilize:
1.  **ResLens:** For capturing GraphQL queries and execution metrics.
2.  **RAG (Retrieval-Augmented Generation):** To draw on documentation and schema info.
3.  **MCP Server:** To route context data (smart contracts, wallet info, transaction status) to the AI.

## Current Progress: Breaking Ground
We have successfully established the foundation for the system. Here is where we stand as of our mid-term check-in:

### 1. The Infrastructure is Live
We have built the MCP server and connected Nexus with ResilientDB’s APIs. The core data pipeline linking these components is functional. We have tested the system with a public LLM (ChatGPT), and it can successfully explain queries based on existing GraphQL documentation.

### 2. RAG System Integration
Below is a glimpse of our RAG (Retrieval-Augmented Generation) system functioning in the CLI. The system can currently parse documentation to assist with query structure.

*(Insert Screenshot: RAG System Test Results for GraphQL Documentation)*

## What's Next?
We are moving from "Proof of Concept" to "Interactive Tool." Our next milestones focus on three key areas:

* **Deep Context Integration:** We are currently dockerizing the system and finishing the integration with **ResLens**. This will allow the AI to see "live" data—like deployed contract addresses and pending transactions—rather than just static documentation.
* **The User Interface:** We are moving away from the Command Line Interface. We are exploring a dedicated chat window within Nexus or a **VS Code Extension** to provide real-time autocomplete and efficiency analysis while you code.
* **Performance Tuning:** We will be testing the latency of the MCP server to ensure that adding AI analysis doesn't slow down the development workflow.

## Conclusion
By the end of the quarter, GraphQ-LLM aims to transform from a backend prototype into a fully integrated AI tutor. Our goal is to make ResilientDB a more accessible platform for blockchain application development by giving developers an AI partner that understands not just their code, but their data.

You can follow our progress on [GitHub here](#).
