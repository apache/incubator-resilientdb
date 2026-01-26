---
layout: default
title: "LangGraph"
nav_order: 13
has_children: true
---

# Tutorial: LangGraph

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

LangGraph<sup>[View Repo](https://github.com/langchain-ai/langgraph/tree/55f922cf2f3e63600ed8f0d0cd1262a75a991fdc/libs/langgraph/langgraph)</sup> helps you build complex **stateful applications**, like chatbots or agents, using a *graph-based approach*.
You define your application's logic as a series of steps (**Nodes**) connected by transitions (**Edges**) in a **Graph**.
The system manages the application's *shared state* using **Channels** and executes the graph step-by-step with its **Pregel engine**, handling things like branching, interruptions, and saving progress (**Checkpointing**).

```mermaid
flowchart TD
    A0["Pregel Execution Engine"]
    A1["Graph / StateGraph"]
    A2["Channels"]
    A3["Nodes (PregelNode)"]
    A4["Checkpointer (BaseCheckpointSaver)"]
    A5["Control Flow Primitives (Branch, Send, Interrupt)"]
    A0 -- "Executes" --> A1
    A1 -- "Contains" --> A3
    A3 -- "Updates State Via" --> A2
    A0 -- "Manages State Via" --> A2
    A0 -- "Uses Checkpointer" --> A4
    A1 -- "Defines Control Flow With" --> A5
    A5 -- "Directs Execution Of" --> A0
    A4 -- "Saves State Of" --> A2
```