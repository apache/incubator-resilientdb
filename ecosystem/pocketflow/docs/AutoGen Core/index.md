---
layout: default
title: "AutoGen Core"
nav_order: 3
has_children: true
---

# Tutorial: AutoGen Core 

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

AutoGen Core<sup>[View Repo](https://github.com/microsoft/autogen/tree/e45a15766746d95f8cfaaa705b0371267bec812e/python/packages/autogen-core/src/autogen_core)</sup> helps you build applications with multiple **_Agents_** that can work together.
Think of it like creating a team of specialized workers (*Agents*) who can communicate and use tools to solve problems.
The **_AgentRuntime_** acts as the manager, handling messages and agent lifecycles.
Agents communicate using a **_Messaging System_** (Topics and Subscriptions), can use **_Tools_** for specific tasks, interact with language models via a **_ChatCompletionClient_** while managing conversation history with **_ChatCompletionContext_**, and remember information using **_Memory_**.
**_Components_** provide a standard way to define and configure these building blocks.


```mermaid
flowchart TD
    A0["0: Agent"]
    A1["1: AgentRuntime"]
    A2["2: Messaging System (Topic & Subscription)"]
    A3["3: Component"]
    A4["4: Tool"]
    A5["5: ChatCompletionClient"]
    A6["6: ChatCompletionContext"]
    A7["7: Memory"]
    A1 -- "Manages lifecycle" --> A0
    A1 -- "Uses for message routing" --> A2
    A0 -- "Uses LLM client" --> A5
    A0 -- "Executes tools" --> A4
    A0 -- "Accesses memory" --> A7
    A5 -- "Gets history from" --> A6
    A5 -- "Uses tool schema" --> A4
    A7 -- "Updates LLM context" --> A6
    A4 -- "Implemented as" --> A3
```
