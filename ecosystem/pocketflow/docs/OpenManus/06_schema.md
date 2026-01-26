---
layout: default
title: "Schema"
parent: "OpenManus"
nav_order: 6
---

# Chapter 6: Schema - The Official Data Forms

In [Chapter 5: BaseFlow](05_baseflow.md), we saw how Flows act like project managers, coordinating different [Agents](03_baseagent.md) and [Tools](04_tool___toolcollection.md) to complete complex tasks. But for all these different parts (Flows, Agents, LLMs, Tools) to work together smoothly, they need to speak the same language and use the same formats when exchanging information.

Imagine a busy office where everyone fills out forms for requests, reports, and messages. If everyone uses their *own* unique form layout, it quickly becomes chaotic! Someone might forget a required field, use the wrong data type (like writing "yesterday" instead of a specific date), or mislabel information. It would be incredibly hard to process anything efficiently.

This is where **Schemas** come into play in OpenManus.

## What Problem Does Schema Solve?

In our digital "office" (the OpenManus application), various components need to pass data back and forth:
*   The User sends a request (a message).
*   The Agent stores this message in its [Memory](02_message___memory.md).
*   The Agent might ask the [LLM](01_llm.md) for help, sending the conversation history.
*   The LLM might decide to use a [Tool](04_tool___toolcollection.md), sending back instructions on which tool and what inputs to use.
*   The Tool executes and sends back its results.
*   The Agent updates its status (e.g., from `RUNNING` to `FINISHED`).

Without a standard way to structure all this information, we'd face problems:
*   **Inconsistency:** One part might expect a user message to have a `sender` field, while another expects a `role` field.
*   **Errors:** A Tool might expect a number as input but receive text, causing it to crash.
*   **Confusion:** It would be hard for developers (and the system itself!) to know exactly what information is contained in a piece of data.
*   **Maintenance Nightmares:** Changing how data is structured in one place could break many other parts unexpectedly.

**Schemas solve this by defining the official "forms" or "templates" for all the important data structures used in OpenManus.** Think of them as the agreed-upon standard formats that everyone must use.

**Use Case:** When the LLM decides the agent should use the `web_search` tool with the query "latest AI news", it doesn't just send back a vague text string. It needs to send structured data that clearly says:
1.  "I want to call a tool."
2.  "The tool's name is `web_search`."
3.  "The input parameter `query` should be set to `latest AI news`."

A schema defines exactly how this "tool call request" should look, ensuring the Agent understands it correctly.

## Key Concepts: Standard Templates via Pydantic

1.  **Schema as Templates:** At its core, a schema is a formal definition of a data structure. It specifies:
    *   What pieces of information (fields) must be included (e.g., a `Message` must have a `role`).
    *   What type each piece of information should be (e.g., `role` must be text, `current_step` in an Agent must be a number).
    *   Which fields are optional and which are required.
    *   Sometimes, default values or specific allowed values (e.g., `role` must be one of "user", "assistant", "system", or "tool").

2.  **Pydantic: The Schema Engine:** OpenManus uses a popular Python library called **Pydantic** to define and enforce these schemas. You don't need to be a Pydantic expert, but understanding its role is helpful. Pydantic lets us define these data structures using simple Python classes. When data is loaded into these classes, Pydantic automatically:
    *   **Validates** the data: Checks if all required fields are present and if the data types are correct. If not, it raises an error *before* the bad data can cause problems elsewhere.
    *   **Provides Auto-completion and Clarity:** Because the structure is clearly defined in code, developers get better auto-completion hints in their editors, making the code easier to write and understand.

Think of Pydantic as the strict office manager who checks every form submitted, ensuring it's filled out correctly according to the official template before passing it on.

## How Do We Use Schemas? (Examples)

Schemas are defined throughout the OpenManus codebase, primarily as Pydantic models. You've already encountered some! Let's look at a few key examples found mostly in `app/schema.py` and `app/tool/base.py`.

**1. `Message` (from `app/schema.py`): The Chat Bubble**

We saw this in [Chapter 2: Message / Memory](02_message___memory.md). It defines the structure for a single turn in a conversation.

```python
# Simplified Pydantic model from app/schema.py
from pydantic import BaseModel, Field
from typing import List, Optional, Literal

# Define allowed roles
ROLE_TYPE = Literal["system", "user", "assistant", "tool"]

class Message(BaseModel):
    role: ROLE_TYPE = Field(...) # '...' means this field is required
    content: Optional[str] = Field(default=None) # Optional text content
    # ... other optional fields like tool_calls, name, tool_call_id ...

    # Class methods like user_message, assistant_message are here...
```

**Explanation:**
*   This Pydantic class `Message` defines the "form" for a message.
*   `role: ROLE_TYPE = Field(...)` means every message *must* have a `role`, and its value must be one of the strings defined in `ROLE_TYPE`. Pydantic enforces this.
*   `content: Optional[str] = Field(default=None)` means a message *can* have text `content`, but it's optional. If not provided, it defaults to `None`.
*   Pydantic ensures that if you try to create a `Message` object without a valid `role`, or with `content` that isn't a string, you'll get an error immediately.

**2. `ToolCall` and `Function` (from `app/schema.py`): The Tool Request Form**

When the LLM tells the agent to use a tool, it sends back data structured according to the `ToolCall` schema.

```python
# Simplified Pydantic models from app/schema.py
from pydantic import BaseModel

class Function(BaseModel):
    name: str      # The name of the tool/function to call
    arguments: str # The input arguments as a JSON string

class ToolCall(BaseModel):
    id: str              # A unique ID for this specific call
    type: str = "function" # Currently always "function"
    function: Function   # Embeds the Function details above
```

**Explanation:**
*   The `Function` schema defines that we need the `name` of the tool (as text) and its `arguments` (also as text, expected to be formatted as JSON).
*   The `ToolCall` schema includes a unique `id`, the `type` (always "function" for now), and embeds the `Function` data.
*   This ensures that whenever the agent receives a tool call instruction from the LLM, it knows exactly where to find the tool's name and arguments, preventing guesswork and errors.

**3. `AgentState` (from `app/schema.py`): The Agent Status Report**

We saw this in [Chapter 3: BaseAgent](03_baseagent.md). It standardizes how we represent the agent's current status.

```python
# Simplified definition from app/schema.py
from enum import Enum

class AgentState(str, Enum):
    """Agent execution states"""
    IDLE = "IDLE"
    RUNNING = "RUNNING"
    FINISHED = "FINISHED"
    ERROR = "ERROR"
```

**Explanation:**
*   This uses Python's `Enum` (Enumeration) type, which is automatically compatible with Pydantic.
*   It defines a fixed set of allowed values for the agent's state. An agent's state *must* be one of these four strings.
*   This prevents typos (like "Runing" or "Idle") and makes it easy to check the agent's status reliably.

**4. `ToolResult` (from `app/tool/base.py`): The Tool Output Form**

When a [Tool](04_tool___toolcollection.md) finishes its job, it needs to report back its findings in a standard way.

```python
# Simplified Pydantic model from app/tool/base.py
from pydantic import BaseModel, Field
from typing import Any, Optional

class ToolResult(BaseModel):
    """Represents the result of a tool execution."""
    output: Any = Field(default=None)          # The main result data
    error: Optional[str] = Field(default=None) # Error message, if any
    # ... other optional fields like base64_image, system message ...

    class Config:
        arbitrary_types_allowed = True # Allows 'Any' type for output
```

**Explanation:**
*   Defines a standard structure for *any* tool's output.
*   It includes an `output` field for the successful result (which can be of `Any` type, allowing flexibility for different tools) and an optional `error` field to report problems.
*   Specific tools might *inherit* from `ToolResult` to add more specific fields, like `SearchResult` adding `url`, `title`, etc. (see `app/tool/web_search.py`). Using `ToolResult` as a base ensures all tool outputs have a consistent minimum structure.

## Under the Hood: Pydantic Validation

The real power of using Pydantic for schemas comes from its automatic data validation. Let's illustrate with a simplified `Message` example.

Imagine you have this Pydantic model:

```python
# Standalone Example (Illustrative)
from pydantic import BaseModel, ValidationError
from typing import Literal

ROLE_TYPE = Literal["user", "assistant"] # Only allow these roles

class SimpleMessage(BaseModel):
    role: ROLE_TYPE
    content: str
```

Now, let's see what happens when we try to create instances:

```python
# --- Valid Data ---
try:
    msg1 = SimpleMessage(role="user", content="Hello there!")
    print("msg1 created successfully:", msg1.model_dump()) # .model_dump() shows dict
except ValidationError as e:
    print("Error creating msg1:", e)

# --- Missing Required Field ('content') ---
try:
    msg2 = SimpleMessage(role="assistant")
    print("msg2 created successfully:", msg2.model_dump())
except ValidationError as e:
    print("\nError creating msg2:")
    print(e) # Pydantic gives a detailed error

# --- Invalid Role ---
try:
    msg3 = SimpleMessage(role="system", content="System message") # 'system' is not allowed
    print("msg3 created successfully:", msg3.model_dump())
except ValidationError as e:
    print("\nError creating msg3:")
    print(e) # Pydantic catches the wrong role

# --- Wrong Data Type for 'content' ---
try:
    msg4 = SimpleMessage(role="user", content=123) # content should be string
    print("msg4 created successfully:", msg4.model_dump())
except ValidationError as e:
    print("\nError creating msg4:")
    print(e) # Pydantic catches the type error
```

**Example Output:**

```
msg1 created successfully: {'role': 'user', 'content': 'Hello there!'}

Error creating msg2:
1 validation error for SimpleMessage
content
  Field required [type=missing, input_value={'role': 'assistant'}, input_type=dict]
    For further information visit https://errors.pydantic.dev/2.7/v/missing

Error creating msg3:
1 validation error for SimpleMessage
role
  Input should be 'user' or 'assistant' [type=literal_error, input_value='system', input_type=str]
    For further information visit https://errors.pydantic.dev/2.7/v/literal_error

Error creating msg4:
1 validation error for SimpleMessage
content
  Input should be a valid string [type=string_type, input_value=123, input_type=int]
    For further information visit https://errors.pydantic.dev/2.7/v/string_type
```

**Explanation:**
*   When the data matches the schema (`msg1`), the object is created successfully.
*   When data is missing (`msg2`), has an invalid value (`msg3`), or the wrong type (`msg4`), Pydantic automatically raises a `ValidationError`.
*   The error message clearly explains *what* is wrong and *where*.

This validation happens automatically whenever data is loaded into these Pydantic models within OpenManus, catching errors early and ensuring data consistency across the entire application. You mostly find these schema definitions in `app/schema.py`, but also within specific tool files (like `app/tool/base.py`, `app/tool/web_search.py`) for their specific results.

## Wrapping Up Chapter 6

You've learned that **Schemas** are like official data templates or forms used throughout OpenManus. They define the expected structure for important data like messages, tool calls, agent states, and tool results. By using the **Pydantic** library, OpenManus automatically **validates** data against these schemas, ensuring consistency, preventing errors, and making the whole system more reliable and easier to understand. They are the backbone of structured communication between different components.

We've now covered most of the core functional building blocks of OpenManus. But how do we configure things like which LLM model to use, API keys, or which tools an agent should have? That's handled by the Configuration system.

Let's move on to [Chapter 7: Configuration (Config)](07_configuration__config_.md) to see how we manage settings and secrets for our agents and flows.

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)