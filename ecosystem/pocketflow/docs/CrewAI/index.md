---
layout: default
title: "CrewAI"
nav_order: 8
has_children: true
---

# Tutorial: CrewAI

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

**CrewAI**<sup>[View Repo](https://github.com/crewAIInc/crewAI/tree/e723e5ca3fb7e4cb890c4befda47746aedbd7408/src/crewai)</sup> is a framework for orchestrating *autonomous AI agents*.
Think of it like building a specialized team (a **Crew**) where each member (**Agent**) has a role, goal, and tools.
You assign **Tasks** to Agents, defining what needs to be done. The **Crew** manages how these Agents collaborate, following a specific **Process** (like sequential steps).
Agents use their "brain" (an **LLM**) and can utilize **Tools** (like web search) and access shared **Memory** or external **Knowledge** bases to complete their tasks effectively.

```mermaid
flowchart TD
    A0["Agent"]
    A1["Task"]
    A2["Crew"]
    A3["Tool"]
    A4["Process"]
    A5["LLM"]
    A6["Memory"]
    A7["Knowledge"]
    A2 -- "Manages" --> A0
    A2 -- "Orchestrates" --> A1
    A2 -- "Defines workflow" --> A4
    A2 -- "Manages shared" --> A6
    A0 -- "Executes" --> A1
    A0 -- "Uses" --> A3
    A0 -- "Uses as brain" --> A5
    A0 -- "Queries" --> A7
    A1 -- "Assigned to" --> A0
```