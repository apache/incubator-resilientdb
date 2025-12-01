---
layout: default
title: "Pydantic Core"
nav_order: 18
has_children: true
---

# Tutorial: Pydantic Core

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

Pydantic Core<sup>[View Repo](https://github.com/pydantic/pydantic/tree/6c38dc93f40a47f4d1350adca9ec0d72502e223f/pydantic)</sup> provides the fundamental machinery for **data validation**, **parsing**, and **serialization** in Pydantic. It takes Python *type hints* and uses them to define how data should be structured and processed. Users typically interact with it by defining classes that inherit from `BaseModel`, which automatically gets validation and serialization capabilities based on its annotated fields. Pydantic Core ensures data conforms to the defined types and allows converting between Python objects and formats like JSON efficiently, leveraging Rust for performance.

```mermaid
flowchart TD
    A0["BaseModel"]
    A1["Fields (FieldInfo / Field function)"]
    A2["Core Schema & Validation/Serialization"]
    A3["Configuration (ConfigDict / ConfigWrapper)"]
    A4["Custom Logic (Decorators & Annotated Helpers)"]
    A5["TypeAdapter"]
    A0 -- "Contains and defines" --> A1
    A0 -- "Is configured by" --> A3
    A0 -- "Applies custom logic via" --> A4
    A1 -- "Is converted into" --> A2
    A3 -- "Configures core engine for" --> A2
    A4 -- "Modifies validation/seriali..." --> A2
    A5 -- "Uses core engine for" --> A2
    A5 -- "Can be configured by" --> A3
```