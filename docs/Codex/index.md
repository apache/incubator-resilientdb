---
layout: default
title: "Codex"
nav_order: 5
has_children: true
---

# Tutorial: Codex

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

Codex<sup>[View Repo](https://github.com/openai/codex)</sup> is a command-line interface (CLI) tool that functions as an **AI coding assistant**.
It runs in your terminal, allowing you to chat with an AI model (like *GPT-4o*) to understand, modify, and generate code within your projects.
The tool can read files, apply changes (*patches*), and execute shell commands, prioritizing safety through user **approval policies** and command **sandboxing**. It supports both interactive chat and a non-interactive *single-pass mode* for batch operations.

```mermaid
flowchart TD
    A0["Agent Loop"]
    A1["Terminal UI (Ink Components)"]
    A2["Approval Policy & Security"]
    A3["Command Execution & Sandboxing"]
    A4["Configuration Management"]
    A5["Response & Tool Call Handling"]
    A6["Single-Pass Mode"]
    A7["Input Handling (TextBuffer/Editor)"]
    A0 -- "Drives updates for" --> A1
    A0 -- "Processes responses via" --> A5
    A0 -- "Consults policy from" --> A2
    A0 -- "Loads config using" --> A4
    A1 -- "Uses editor for input" --> A7
    A2 -- "Dictates sandboxing for" --> A3
    A4 -- "Provides settings to" --> A2
    A5 -- "Triggers" --> A3
    A7 -- "Provides user input to" --> A0
    A0 -- "Can initiate" --> A6
    A6 -- "Renders via specific UI" --> A1
```