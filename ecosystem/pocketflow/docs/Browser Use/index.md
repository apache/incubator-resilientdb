---
layout: default
title: "Browser Use"
nav_order: 4
has_children: true
---

# Tutorial: Browser Use

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

**Browser Use**<sup>[View Repo](https://github.com/browser-use/browser-use/tree/3076ba0e83f30b45971af58fe2aeff64472da812/browser_use)</sup> is a project that allows an *AI agent* to control a web browser and perform tasks automatically.
Think of it like an AI assistant that can browse websites, fill forms, click buttons, and extract information based on your instructions. It uses a Large Language Model (LLM) as its "brain" to decide what actions to take on a webpage to complete a given *task*. The project manages the browser session, understands the page structure (DOM), and communicates back and forth with the LLM.

```mermaid
flowchart TD
    A0["Agent"]
    A1["BrowserContext"]
    A2["Action Controller & Registry"]
    A3["DOM Representation"]
    A4["Message Manager"]
    A5["System Prompt"]
    A6["Data Structures (Views)"]
    A7["Telemetry Service"]
    A0 -- "Gets state from" --> A1
    A0 -- "Uses to execute actions" --> A2
    A0 -- "Uses for LLM communication" --> A4
    A0 -- "Gets instructions from" --> A5
    A0 -- "Uses/Produces data formats" --> A6
    A0 -- "Logs events to" --> A7
    A1 -- "Gets DOM structure via" --> A3
    A1 -- "Provides BrowserState" --> A6
    A2 -- "Executes actions on" --> A1
    A2 -- "Defines/Uses ActionModel/Ac..." --> A6
    A2 -- "Logs registered functions to" --> A7
    A3 -- "Provides structure to" --> A1
    A3 -- "Uses DOM structures" --> A6
    A4 -- "Provides messages to" --> A0
    A4 -- "Initializes with" --> A5
    A4 -- "Formats data using" --> A6
    A5 -- "Defines structure for Agent..." --> A6
    A7 -- "Receives events from" --> A0
```
