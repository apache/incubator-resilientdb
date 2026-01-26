---
layout: default
title: "DSPy"
nav_order: 9
has_children: true
---

# Tutorial: DSPy

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

DSPy<sup>[View Repo](https://github.com/stanfordnlp/dspy/tree/7cdfe988e6404289b896d946d957f17bb4d9129b/dspy)</sup> helps you build and optimize *programs* that use **Language Models (LMs)** and **Retrieval Models (RMs)**.
Think of it like composing Lego bricks (**Modules**) where each brick performs a specific task (like generating text or retrieving information).
**Signatures** define what each Module does (its inputs and outputs), and **Teleprompters** automatically tune these modules (like optimizing prompts or examples) to get the best performance on your data.

```mermaid
flowchart TD
    A0["Module / Program"]
    A1["Signature"]
    A2["Predict"]
    A3["LM (Language Model Client)"]
    A4["RM (Retrieval Model Client)"]
    A5["Teleprompter / Optimizer"]
    A6["Example"]
    A7["Evaluate"]
    A8["Adapter"]
    A9["Settings"]
    A0 -- "Contains / Composes" --> A0
    A0 -- "Uses (via Retrieve)" --> A4
    A1 -- "Defines structure for" --> A6
    A2 -- "Implements" --> A1
    A2 -- "Calls" --> A3
    A2 -- "Uses demos from" --> A6
    A2 -- "Formats prompts using" --> A8
    A5 -- "Optimizes" --> A0
    A5 -- "Fine-tunes" --> A3
    A5 -- "Uses training data from" --> A6
    A5 -- "Uses metric from" --> A7
    A7 -- "Tests" --> A0
    A7 -- "Evaluates on dataset of" --> A6
    A8 -- "Translates" --> A1
    A8 -- "Formats demos from" --> A6
    A9 -- "Configures default" --> A3
    A9 -- "Configures default" --> A4
    A9 -- "Configures default" --> A8
```