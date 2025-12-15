---
layout: default
title: "Click"
nav_order: 6
has_children: true
---

# Tutorial: Click

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

Click<sup>[View Repo](https://github.com/pallets/click/tree/main/src/click)</sup> is a Python library that makes creating **command-line interfaces (CLIs)** *easy and fun*.
It uses simple Python **decorators** (`@click.command`, `@click.option`, etc.) to turn your functions into CLI commands with options and arguments.
Click handles parsing user input, generating help messages, validating data types, and managing the flow between commands, letting you focus on your application's logic.
It also provides tools for *terminal interactions* like prompting users and showing progress bars.


```mermaid
flowchart TD
    A0["Context"]
    A1["Command / Group"]
    A2["Parameter (Option / Argument)"]
    A3["ParamType"]
    A4["Decorators"]
    A5["Term UI (Terminal User Interface)"]
    A6["Click Exceptions"]
    A4 -- "Creates/Configures" --> A1
    A4 -- "Creates/Configures" --> A2
    A0 -- "Manages execution of" --> A1
    A0 -- "Holds parsed values for" --> A2
    A2 -- "Uses for validation/conversion" --> A3
    A3 -- "Raises on conversion error" --> A6
    A1 -- "Uses for user interaction" --> A5
    A0 -- "Handles/Raises" --> A6
    A4 -- "Injects via @pass_context" --> A0
```

