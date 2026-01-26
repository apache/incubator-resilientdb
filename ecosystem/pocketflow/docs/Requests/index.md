---
layout: default
title: "Requests"
nav_order: 19
has_children: true
---

# Tutorial: Requests

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

Requests<sup>[View Repo](https://github.com/psf/requests/tree/0e322af87745eff34caffe4df68456ebc20d9068/src/requests)</sup> is a Python library that makes sending *HTTP requests* incredibly simple.
Instead of dealing with complex details, you can use straightforward functions (like `requests.get()`) or **Session objects** to interact with web services.
It automatically handles things like *cookies*, *redirects*, *authentication*, and connection pooling, returning easy-to-use **Response objects** with all the server's data.

```mermaid
flowchart TD
    A0["Request & Response Models"]
    A1["Session"]
    A2["Transport Adapters"]
    A3["Functional API"]
    A4["Authentication Handlers"]
    A5["Cookie Jar"]
    A6["Exception Hierarchy"]
    A7["Hook System"]
    A3 -- "Uses temporary" --> A1
    A1 -- "Prepares/Receives" --> A0
    A1 -- "Manages & Uses" --> A2
    A1 -- "Manages" --> A5
    A1 -- "Manages" --> A4
    A1 -- "Manages" --> A7
    A2 -- "Sends/Builds" --> A0
    A4 -- "Modifies (adds headers)" --> A0
    A5 -- "Populates/Reads" --> A0
    A7 -- "Operates on" --> A0
    A0 -- "Can Raise (raise_for_status)" --> A6
    A2 -- "Raises Connection Errors" --> A6
```