---
layout: default
title: "Google A2A"
nav_order: 12
has_children: true
---

# Tutorial: Google A2A

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

The **Google A2A (Agent-to-Agent)**<sup>[View Repo](https://github.com/google/A2A)</sup> project defines an *open protocol* enabling different AI agents, possibly built with different technologies, to communicate and work together.
Think of it as a common language (*A2A Protocol*) agents use to discover each other (*Agent Card*), assign work (*Task*), and exchange results, even providing real-time updates (*Streaming*).
The project includes sample *client* and *server* implementations, example agents using frameworks like LangGraph or CrewAI, and a *demo UI* showcasing multi-agent interactions.

```mermaid
flowchart TD
    A0["A2A Protocol & Core Types"]
    A1["Task"]
    A2["Agent Card"]
    A3["A2A Server Implementation"]
    A4["A2A Client Implementation"]
    A5["Task Handling Logic (Server-side)"]
    A6["Streaming Communication (SSE)"]
    A7["Demo UI Application & Service"]
    A8["Multi-Agent Orchestration (Host Agent)"]
    A0 -- "Defines Structure For" --> A1
    A0 -- "Defines Structure For" --> A2
    A4 -- "Sends Task Requests To" --> A3
    A3 -- "Delegates Task To" --> A5
    A5 -- "Executes" --> A1
    A8 -- "Uses for Discovery" --> A2
    A3 -- "Sends Updates Via" --> A6
    A4 -- "Receives Updates Via" --> A6
    A8 -- "Acts As" --> A4
    A7 -- "Presents/Manages" --> A8
    A7 -- "Communicates With" --> A5
```