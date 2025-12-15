---
layout: default
title: "Celery"
nav_order: 5
has_children: true
---

# Tutorial: Celery

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

Celery<sup>[View Repo](https://github.com/celery/celery/tree/d1c35bbdf014f13f4ab698d75e3ea381a017b090/celery)</sup> is a system for running **distributed tasks** *asynchronously*. You define *units of work* (Tasks) in your Python code. When you want a task to run, you send a message using a **message broker** (like RabbitMQ or Redis). One or more **Worker** processes are running in the background, listening for these messages. When a worker receives a message, it executes the corresponding task. Optionally, the task's result (or any error) can be stored in a **Result Backend** (like Redis or a database) so you can check its status or retrieve the output later. Celery helps manage this whole process, making it easier to handle background jobs, scheduled tasks, and complex workflows.

```mermaid
flowchart TD
    A0["Celery App"]
    A1["Task"]
    A2["Worker"]
    A3["Broker Connection (AMQP)"]
    A4["Result Backend"]
    A5["Canvas (Signatures & Primitives)"]
    A6["Beat (Scheduler)"]
    A7["Configuration"]
    A8["Events"]
    A9["Bootsteps"]
    A0 -- "Defines and sends" --> A1
    A0 -- "Uses for messaging" --> A3
    A0 -- "Uses for results" --> A4
    A0 -- "Loads and uses" --> A7
    A1 -- "Updates state in" --> A4
    A2 -- "Executes" --> A1
    A2 -- "Fetches tasks from" --> A3
    A2 -- "Uses for lifecycle" --> A9
    A5 -- "Represents task invocation" --> A1
    A6 -- "Sends scheduled tasks via" --> A3
    A8 -- "Sends events via" --> A3
    A9 -- "Manages connection via" --> A3
```

