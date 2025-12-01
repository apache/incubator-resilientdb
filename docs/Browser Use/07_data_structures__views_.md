---
layout: default
title: "Data Structures (Views)"
parent: "Browser Use"
nav_order: 7
---

# Chapter 7: Data Structures (Views) - The Project's Blueprints

In the [previous chapter](06_message_manager.md), we saw how the `MessageManager` acts like a secretary, carefully organizing the conversation between the [Agent](01_agent.md) and the LLM. It manages different pieces of information – the browser's current state, the LLM's plan, the results of actions, and more.

But how do all these different components – the Agent, the LLM parser, the [BrowserContext](03_browsercontext.md), the [Action Controller & Registry](05_action_controller___registry.md), and the [Message Manager](06_message_manager.md) – ensure they understand each other perfectly? If the LLM gives a plan in one format, and the Controller expects it in another, things will break!

Imagine trying to build furniture using instructions written in a language you don't fully understand, or trying to fill out a form where every section uses a different layout. It would be confusing and error-prone. We need a shared, consistent language and format.

This is where **Data Structures (Views)** come in. They act as the official blueprints or standardized forms for all the important information passed around within the `Browser Use` project.

## What Problem Do Data Structures Solve?

In a complex system like `Browser Use`, many components need to exchange data:

*   The [BrowserContext](03_browsercontext.md) needs to package up the current state of the webpage.
*   The [Agent](01_agent.md) needs to understand the LLM's multi-step plan.
*   The [Action Controller & Registry](05_action_controller___registry.md) needs to know exactly which action to perform and with what specific parameters (like which element index to click).
*   The Controller needs to report back the result of an action in a predictable way.

Without a standard format for each piece of data, you might encounter problems like:

*   Misinterpreting data (e.g., is `5` an element index or a quantity?).
*   Missing required information.
*   Inconsistent naming (`element_id` vs `index` vs `element_number`).
*   Difficulty debugging when data looks different every time.

Data Structures (Views) solve this by defining **strict, consistent blueprints** for the data. Everyone agrees to use these blueprints, ensuring smooth communication and preventing errors.

## Meet Pydantic: The Blueprint Maker and Checker

In `Browser Use`, these blueprints are primarily defined using a popular Python library called **Pydantic**.

Think of Pydantic like a combination of:

1.  **A Blueprint Designer:** It provides an easy way to define the structure of your data using standard Python type hints (like `str` for text, `int` for whole numbers, `bool` for True/False, `list` for lists).
2.  **A Quality Inspector:** When data comes in (e.g., from the LLM or from an action's result), Pydantic automatically checks if it matches the blueprint. Does it have all the required fields? Are the data types correct? If not, Pydantic raises an error, stopping bad data before it causes problems later.

These Pydantic models (our blueprints) are often stored in files named `views.py` within different component directories (like `agent/views.py`, `browser/views.py`), which is why we sometimes call them "Views".

## Key Blueprints in `Browser Use`

Let's look at some of the most important data structures used in the project. Don't worry about memorizing every detail; focus on *what kind* of information each blueprint holds and *who* uses it.

*(Note: These are simplified representations. The actual models might have more fields or features.)*

### 1. `BrowserState` (from `browser/views.py`)

*   **Purpose:** Represents a complete snapshot of the browser's state at a specific moment.
*   **Blueprint Contents (Simplified):**
    *   `url`: The current web address (string).
    *   `title`: The title of the webpage (string).
    *   `element_tree`: The simplified map of the webpage content (from [DOM Representation](04_dom_representation.md)).
    *   `selector_map`: The lookup map for interactive elements (from [DOM Representation](04_dom_representation.md)).
    *   `screenshot`: An optional image of the page (string, base64 encoded).
    *   `tabs`: Information about other open tabs in this context (list).
*   **Who Uses It:**
    *   Created by: [BrowserContext](03_browsercontext.md) (`get_state()` method).
    *   Used by: [Agent](01_agent.md) (to see the current situation), [Message Manager](06_message_manager.md) (to store in history).

```python
# --- Conceptual Pydantic Model ---
# File: browser/views.py (Simplified Example)
from pydantic import BaseModel
from typing import Optional, List, Dict # For type hints
# Assume DOMElementNode and TabInfo are defined elsewhere

class BrowserState(BaseModel):
    url: str
    title: str
    element_tree: Optional[object] # Simplified: Actual type is DOMElementNode
    selector_map: Optional[Dict[int, object]] # Simplified: Actual type is SelectorMap
    screenshot: Optional[str] = None # Optional field
    tabs: List[object] = [] # Simplified: Actual type is TabInfo

# Pydantic ensures that when a BrowserState is created,
# 'url' and 'title' MUST be provided as strings.
```

### 2. `ActionModel` (from `controller/registry/views.py`)

*   **Purpose:** Represents a *single* specific action the LLM wants to perform, including its parameters. This model is often created *dynamically* based on the actions available in the [Action Controller & Registry](05_action_controller___registry.md).
*   **Blueprint Contents (Example for `click_element`):**
    *   `index`: The `highlight_index` of the element to click (integer).
    *   `xpath`: An optional hint about the element's location (string).
*   **Blueprint Contents (Example for `input_text`):**
    *   `index`: The `highlight_index` of the input field (integer).
    *   `text`: The text to type (string).
*   **Who Uses It:**
    *   Defined by/Registered in: [Action Controller & Registry](05_action_controller___registry.md).
    *   Created based on: LLM output (often part of `AgentOutput`).
    *   Used by: [Action Controller & Registry](05_action_controller___registry.md) (to validate parameters and know what function to call).

```python
# --- Conceptual Pydantic Models ---
# File: controller/views.py (Simplified Examples)
from pydantic import BaseModel
from typing import Optional

class ClickElementAction(BaseModel):
    index: int
    xpath: Optional[str] = None # Optional hint

class InputTextAction(BaseModel):
    index: int
    text: str
    xpath: Optional[str] = None # Optional hint

# Base model that dynamically holds ONE of the above actions
class ActionModel(BaseModel):
    # Pydantic allows models like this where only one field is expected
    # e.g., ActionModel(click_element=ClickElementAction(index=5))
    # or    ActionModel(input_text=InputTextAction(index=12, text="hello"))
    click_element: Optional[ClickElementAction] = None
    input_text: Optional[InputTextAction] = None
    # ... fields for other possible actions (scroll, done, etc.) ...
    pass # More complex logic handles ensuring only one action is present
```

### 3. `AgentOutput` (from `agent/views.py`)

*   **Purpose:** Represents the complete plan received from the LLM after it analyzes the current state. This is the structure the [System Prompt](02_system_prompt.md) tells the LLM to follow.
*   **Blueprint Contents (Simplified):**
    *   `current_state`: The LLM's thoughts/reasoning (a nested structure, often called `AgentBrain`).
    *   `action`: A *list* of one or more `ActionModel` objects representing the steps the LLM wants to take.
*   **Who Uses It:**
    *   Created by: The [Agent](01_agent.md) parses the LLM's raw JSON output into this structure.
    *   Used by: [Agent](01_agent.md) (to understand the plan), [Message Manager](06_message_manager.md) (to store the plan in history), [Action Controller & Registry](05_action_controller___registry.md) (reads the `action` list).

```python
# --- Conceptual Pydantic Model ---
# File: agent/views.py (Simplified Example)
from pydantic import BaseModel
from typing import List
# Assume ActionModel and AgentBrain are defined elsewhere

class AgentOutput(BaseModel):
    current_state: object # Simplified: Actual type is AgentBrain
    action: List[ActionModel] # A list of actions to execute

# Pydantic ensures the LLM output MUST have 'current_state' and 'action',
# and that 'action' MUST be a list containing valid ActionModel objects.
```

### 4. `ActionResult` (from `agent/views.py`)

*   **Purpose:** Represents the outcome after the [Action Controller & Registry](05_action_controller___registry.md) attempts to execute a single action.
*   **Blueprint Contents (Simplified):**
    *   `is_done`: Did this action signal the end of the overall task? (boolean, optional).
    *   `success`: If done, was the task successful overall? (boolean, optional).
    *   `extracted_content`: Any text result from the action (e.g., "Clicked button X") (string, optional).
    *   `error`: Any error message if the action failed (string, optional).
    *   `include_in_memory`: Should this result be explicitly shown to the LLM next time? (boolean).
*   **Who Uses It:**
    *   Created by: Functions within the [Action Controller & Registry](05_action_controller___registry.md) (like `click_element`).
    *   Used by: [Agent](01_agent.md) (to check status, record results), [Message Manager](06_message_manager.md) (includes info in the next state message sent to LLM).

```python
# --- Conceptual Pydantic Model ---
# File: agent/views.py (Simplified Example)
from pydantic import BaseModel
from typing import Optional

class ActionResult(BaseModel):
    is_done: Optional[bool] = False
    success: Optional[bool] = None
    extracted_content: Optional[str] = None
    error: Optional[str] = None
    include_in_memory: bool = False # Default to False

# Pydantic helps ensure results are consistently structured.
# For example, 'is_done' must be True or False if provided.
```

## The Power of Blueprints: Ensuring Consistency

Using Pydantic models for these data structures provides a huge benefit: **automatic validation**.

Imagine the LLM sends back a plan, but it forgets to include the `index` for a `click_element` action.

```json
// Bad LLM Response (Missing 'index')
{
  "current_state": { ... },
  "action": [
    {
      "click_element": {
         "xpath": "//button[@id='submit']" // 'index' is missing!
      }
    }
  ]
}
```

When the [Agent](01_agent.md) tries to parse this JSON into the `AgentOutput` Pydantic model, Pydantic will immediately notice that the `index` field (which is required by the `ClickElementAction` blueprint) is missing. It will raise a `ValidationError`.

```python
# --- Conceptual Agent Code ---
import pydantic
# Assume AgentOutput is the Pydantic model defined earlier
# Assume 'llm_json_response' contains the bad JSON from above

try:
    # Try to create the AgentOutput object from the LLM's response
    llm_plan = AgentOutput.model_validate_json(llm_json_response)
    # If validation succeeds, proceed...
    print("LLM Plan Validated:", llm_plan)
except pydantic.ValidationError as e:
    # Pydantic catches the error!
    print(f"Validation Error: The LLM response didn't match the blueprint!")
    print(e)
    # The Agent can now handle this error gracefully,
    # maybe asking the LLM to try again, instead of crashing later.
```

This automatic checking catches errors early, preventing the [Action Controller & Registry](05_action_controller___registry.md) from receiving incomplete instructions and making the whole system much more robust and easier to debug. It enforces the "contract" between different components.

## Under the Hood: Simple Classes

These data structures are simply Python classes, mostly inheriting from `pydantic.BaseModel` or defined using Python's built-in `dataclass`. They don't contain complex logic themselves; their main job is to define the *shape* and *type* of the data. You'll find their definitions scattered across the various `views.py` files within the project's component directories (like `agent/`, `browser/`, `controller/`, `dom/`).

Think of them as the official vocabulary and grammar rules that all the components agree to use when communicating.

## Conclusion

Data Structures (Views), primarily defined using Pydantic models, are the essential blueprints that ensure consistent and reliable communication within the `Browser Use` project. They act like standardized forms for `BrowserState`, `AgentOutput`, `ActionModel`, and `ActionResult`, making sure every component knows exactly what kind of data to expect and how to interpret it.

By defining these clear structures and leveraging Pydantic's automatic validation, `Browser Use` prevents misunderstandings between components, catches errors early, and makes the overall system more robust and maintainable. These standardized structures also make it easier to log and understand what's happening in the system.

Speaking of logging and understanding the system's behavior, how can we monitor the Agent's performance and gather data for improvement? In the next and final chapter, we'll explore the [Telemetry Service](08_telemetry_service.md).

[Next Chapter: Telemetry Service](08_telemetry_service.md)

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)