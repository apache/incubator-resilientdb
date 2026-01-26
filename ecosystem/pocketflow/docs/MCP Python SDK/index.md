---
layout: default
title: "MCP Python SDK"
nav_order: 15
has_children: true
---

# Tutorial: MCP Python SDK

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

The **MCP Python SDK**<sup>[View Repo](https://github.com/modelcontextprotocol/python-sdk/tree/d788424caa43599de38cee2f70233282d83e3a34/src/mcp)</sup> helps developers build applications (clients and servers) that talk to each other using the *Model Context Protocol (MCP)* specification.
It simplifies communication by handling the low-level details like standard **message formats** (Abstraction 0), connection **sessions** (Abstraction 1), and different ways to send/receive data (**transports**, Abstraction 2).
It also provides a high-level framework, **`FastMCP`** (Abstraction 3), making it easy to create servers that expose **tools** (Abstraction 5), **resources** (Abstraction 4), and **prompts** (Abstraction 6) to clients.
The SDK includes **command-line tools** (Abstraction 8) for running and managing these servers.

```mermaid
flowchart TD
    A0["MCP Protocol Types"]
    A1["Client/Server Sessions"]
    A2["Communication Transports"]
    A3["FastMCP Server"]
    A4["FastMCP Resources"]
    A5["FastMCP Tools"]
    A6["FastMCP Prompts"]
    A7["FastMCP Context"]
    A8["CLI"]
    A1 -- "Uses MCP Types" --> A0
    A1 -- "Operates Over Transport" --> A2
    A2 -- "Serializes/Deserializes MCP..." --> A0
    A3 -- "Uses Session Logic" --> A1
    A3 -- "Manages Resources" --> A4
    A3 -- "Manages Tools" --> A5
    A3 -- "Manages Prompts" --> A6
    A8 -- "Runs/Configures Server" --> A3
    A5 -- "Handlers Can Use Context" --> A7
    A4 -- "Handlers Can Use Context" --> A7
    A7 -- "Provides Access To Session" --> A1
    A7 -- "Provides Access To Server" --> A3
```
