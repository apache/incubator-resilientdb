---
layout: default
title: "OpenManus"
nav_order: 17
has_children: true
---

# Tutorial: OpenManus

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

OpenManus<sup>[View Repo](https://github.com/mannaandpoem/OpenManus/tree/f616c5d43d02d93ccc6e55f11666726d6645fdc2)</sup> is a framework for building autonomous *AI agents*.
Think of it like a digital assistant that can perform tasks. It uses a central **brain** (an `LLM` like GPT-4) to understand requests and decide what to do next.
Agents can use various **tools** (like searching the web or writing code) to interact with the world or perform specific actions. Some complex tasks might involve a **flow** that coordinates multiple agents.
It keeps track of the conversation using `Memory` and ensures secure code execution using a `DockerSandbox`.
The system is flexible, allowing new tools to be added, even dynamically through the `MCP` protocol.

```mermaid
flowchart TD
    A0["BaseAgent"]
    A1["Tool / ToolCollection"]
    A2["LLM"]
    A3["Message / Memory"]
    A4["Schema"]
    A5["BaseFlow"]
    A6["DockerSandbox"]
    A7["Configuration (Config)"]
    A8["MCP (Model Context Protocol)"]
    A0 -- "Uses LLM for thinking" --> A2
    A0 -- "Uses Memory for context" --> A3
    A0 -- "Executes Tools" --> A1
    A5 -- "Orchestrates Agents" --> A0
    A1 -- "Uses Sandbox for execution" --> A6
    A2 -- "Reads LLM Config" --> A7
    A6 -- "Reads Sandbox Config" --> A7
    A7 -- "Provides MCP Config" --> A8
    A8 -- "Provides Dynamic Tools" --> A1
    A8 -- "Extends BaseAgent" --> A0
    A4 -- "Defines Agent Structures" --> A0
    A4 -- "Defines Message Structure" --> A3
    A2 -- "Processes Messages" --> A3
    A5 -- "Uses Tools" --> A1
    A4 -- "Defines Tool Structures" --> A1
```
