---
layout: default
title: "AgentLogger & Monitor"
parent: "SmolaAgents"
nav_order: 8
---

# Chapter 8: AgentLogger & Monitor - Observing Your Agent in Action

Welcome to the final chapter of the SmolaAgents tutorial! In [Chapter 7: AgentType](07_agenttype.md), we saw how `SmolaAgents` handles different kinds of data like text, images, and audio using specialized containers. Now that our agent can perform complex tasks ([Chapter 1: MultiStepAgent](01_multistepagent.md)), use various [Tools](03_tool.md), remember its progress ([Chapter 4: AgentMemory](04_agentmemory.md)), and even handle diverse data types, a new question arises: **How do we actually see what the agent is doing?**

What if the agent gets stuck in a loop? What if it uses the wrong tool or gives an unexpected answer? How can we peek inside its "mind" to understand its reasoning, track its actions, and maybe figure out what went wrong or how well it's performing?

## The Problem: Flying Blind

Imagine driving a car with no dashboard. You wouldn't know your speed, fuel level, or if the engine was overheating. You'd be driving blind! Or imagine an airplane without its "black box" flight recorder – after an incident, it would be much harder to understand what happened.

![Car with no dashboard](https://img.icons8.com/ios/50/000000/car--v1.png) ❓❓❓

Running an AI agent without visibility is similar. Without seeing its internal steps, thoughts, and actions, debugging problems or understanding its behavior becomes incredibly difficult. We need a way to observe the agent in real-time and record its performance.

## The Solution: The Dashboard (`AgentLogger`) and Black Box (`Monitor`)

`SmolaAgents` provides two key components to give you this visibility:

1.  **`AgentLogger` (The Dashboard):** This component provides **structured, real-time logging** of the agent's activities directly to your console (or wherever you run your Python script). It uses a library called `rich` to display colorful, formatted output, making it easy to follow:
    *   Which step the agent is on.
    *   The LLM's thoughts and the action it plans to take.
    *   Which [Tool](03_tool.md) is being called and with what arguments.
    *   The results (observations) from the tool.
    *   Any errors encountered.
    It's like watching the car's speedometer, fuel gauge, and warning lights as you drive.

2.  **`Monitor` (The Black Box):** This component works quietly in the background, **tracking key performance metrics** during the agent's run. It records data like:
    *   How long each step took (duration).
    *   How many tokens the LLM used for input and output (if the [Model Interface](02_model_interface.md) provides this).
    This data isn't usually displayed as prominently as the logger's output but is stored and can be used later for analysis, cost calculation, or identifying performance bottlenecks. It's like the airplane's flight data recorder.

Both `AgentLogger` and `Monitor` are automatically set up and used by the `MultiStepAgent`, making observation easy!

## `AgentLogger`: Your Real-Time Dashboard

The `AgentLogger` is your primary window into the agent's live execution. It makes the **Think -> Act -> Observe** cycle visible.

**How It's Used (Automatic!)**

When you create a `MultiStepAgent`, it automatically creates an `AgentLogger` instance, usually stored in `self.logger`. Throughout the agent's `run` process, various methods within the agent call `self.logger` to print information:

*   `agent.run()` calls `self.logger.log_task()` to show the initial task.
*   `agent._execute_step()` calls `self.logger.log_rule()` to mark the beginning of a new step.
*   If the agent uses code (like `CodeAgent`), it calls `self.logger.log_code()` to show the code being executed.
*   It logs tool calls using `self.logger.log()`.
*   It logs observations using `self.logger.log()`.
*   It logs errors using `self.logger.log_error()`.
*   It logs the final answer using `self.logger.log()`.

**Example Output (Simulated)**

The `AgentLogger` uses `rich` to make the output colorful and easy to read. Here's a simplified idea of what you might see in your console for our "Capital and Weather" example:

```console
╭─[bold] New run ─ ToolCallingAgent [/bold]────────────────────────────────╮
│                                                                       │
│ [bold]What is the capital of France, and what is its current weather?[/bold] │
│                                                                       │
╰────────────────────────── LiteLLMModel - gpt-3.5-turbo ─╯

━━━[bold] Step 1 [/bold]━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
INFO     ╭─ Thinking... ───────────────────────────────────────────────────╮
INFO     │ Thought: The user wants the capital of France and its weather.│
INFO     │ First, I need to find the capital. I can use the search tool. │
INFO     ╰─────────────────────────────────────────────────────────────────╯
INFO     Panel(Text("Calling tool: 'search' with arguments: {'query': 'Capital of France'}"))
INFO     Observations: Paris
DEBUG    [Step 1: Duration 1.52 seconds| Input tokens: 150 | Output tokens: 50]

━━━[bold] Step 2 [/bold]━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
INFO     ╭─ Thinking... ───────────────────────────────────────────────────╮
INFO     │ Thought: I have the capital, which is Paris. Now I need the   │
INFO     │ weather for Paris. I can use the weather tool.                │
INFO     ╰─────────────────────────────────────────────────────────────────╯
INFO     Panel(Text("Calling tool: 'weather' with arguments: {'location': 'Paris'}"))
INFO     Observations: Sunny, 25°C
DEBUG    [Step 2: Duration 1.81 seconds| Input tokens: 210 | Output tokens: 105]

━━━[bold] Step 3 [/bold]━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
INFO     ╭─ Thinking... ───────────────────────────────────────────────────╮
INFO     │ Thought: I have both the capital (Paris) and the weather      │
INFO     │ (Sunny, 25°C). I have fulfilled the user's request. I should  │
INFO     │ use the final_answer tool.                                    │
INFO     ╰─────────────────────────────────────────────────────────────────╯
INFO     Panel(Text("Calling tool: 'final_answer' with arguments: {'answer': 'The capital of France is Paris, and the current weather there is Sunny, 25°C.'}"))
INFO     [bold #d4b702]Final answer:[/bold #d4b702] The capital of France is Paris, and the current weather there is Sunny, 25°C.
DEBUG    [Step 3: Duration 1.25 seconds| Input tokens: 280 | Output tokens: 170]
```

*(Note: This is a conceptual representation. The exact formatting, colors, and details might vary. The "Thinking..." part is simulated; the logger typically shows the raw model output or parsed action.)*

**Log Levels**

You can control how much detail the logger shows using the `verbosity_level` parameter when creating the agent:

*   `LogLevel.INFO` (Default): Shows the main steps, tool calls, observations, final answer, and errors. Good for general use.
*   `LogLevel.DEBUG`: Shows everything `INFO` shows, plus the detailed LLM inputs/outputs and performance metrics from the `Monitor`. Useful for deep debugging.
*   `LogLevel.ERROR`: Only shows critical error messages.
*   `LogLevel.OFF`: Shows nothing.

```python
from smolagents import CodeAgent
from smolagents.models import LiteLLMModel
from smolagents.monitoring import LogLevel # Import LogLevel

llm = LiteLLMModel(model_id="gpt-3.5-turbo")

# Create an agent with DEBUG level logging
agent_debug = CodeAgent(
    model=llm,
    tools=[],
    verbosity_level=LogLevel.DEBUG # Set the level here
)

# This agent will print more detailed logs when run
# agent_debug.run("What is 2+2?")
```

**Code Glimpse (`monitoring.py` and `agents.py`)**

*   **`AgentLogger` Class:** It uses the `rich.console.Console` to print formatted output based on the log level.

    ```python
    # --- File: monitoring.py (Simplified AgentLogger) ---
    from enum import IntEnum
    from rich.console import Console
    from rich.panel import Panel
    from rich.syntax import Syntax
    from rich.rule import Rule
    # ... other rich imports ...

    class LogLevel(IntEnum):
        OFF = -1
        ERROR = 0
        INFO = 1
        DEBUG = 2

    YELLOW_HEX = "#d4b702" # Used for styling

    class AgentLogger:
        def __init__(self, level: LogLevel = LogLevel.INFO):
            self.level = level
            # The core object from the 'rich' library for printing
            self.console = Console()

        def log(self, *args, level: LogLevel = LogLevel.INFO, **kwargs):
            """Logs a message if the level is sufficient."""
            if level <= self.level:
                self.console.print(*args, **kwargs)

        def log_error(self, error_message: str):
            """Logs an error message."""
            self.log(error_message, style="bold red", level=LogLevel.ERROR)

        def log_code(self, title: str, content: str, level: LogLevel = LogLevel.INFO):
            """Logs a Python code block with syntax highlighting."""
            self.log(
                Panel(Syntax(content, lexer="python", ...), title=title, ...),
                level=level
            )

        def log_rule(self, title: str, level: LogLevel = LogLevel.INFO):
            """Logs a horizontal rule separator."""
            self.log(Rule("[bold]" + title, style=YELLOW_HEX), level=level)

        def log_task(self, content: str, subtitle: str, title: Optional[str] = None, level: LogLevel = LogLevel.INFO):
             """Logs the initial task."""
             self.log(Panel(f"\n[bold]{content}\n", title=title, subtitle=subtitle, ...), level=level)

        # ... other helper methods for specific formatting ...
    ```

*   **Agent Using the Logger:** The `MultiStepAgent` calls `self.logger` methods.

    ```python
    # --- File: agents.py (Simplified Agent using Logger) ---
    from .monitoring import AgentLogger, LogLevel

    class MultiStepAgent:
        def __init__(self, ..., verbosity_level: LogLevel = LogLevel.INFO):
            # ... other setup ...
            self.logger = AgentLogger(level=verbosity_level)
            # ...

        def run(self, task: str, ...):
            # ...
            self.logger.log_task(content=self.task, ..., level=LogLevel.INFO)
            # ... call _run ...

        def _execute_step(self, task: str, memory_step: ActionStep):
            self.logger.log_rule(f"Step {self.step_number}", level=LogLevel.INFO)
            try:
                # ... (Think phase: LLM call) ...

                # ... (Act phase: Execute tool/code) ...
                # Example for CodeAgent:
                # self.logger.log_code("Executing code:", code_action, level=LogLevel.INFO)
                # observation = self.python_executor(code_action)

                # Example for ToolCallingAgent:
                # self.logger.log(Panel(f"Calling tool: '{tool_name}' ..."), level=LogLevel.INFO)
                # observation = self.execute_tool_call(tool_name, arguments)

                # ... (Observe phase) ...
                self.logger.log(f"Observations: {observation}", level=LogLevel.INFO)

                # ... (Handle final answer) ...
                # if final_answer:
                #    self.logger.log(f"Final answer: {final_answer}", style=f"bold {YELLOW_HEX}", level=LogLevel.INFO)

            except AgentError as e:
                # Log errors using the logger's error method
                action_step.error = e # Store error in memory
                self.logger.log_error(f"Error in step {self.step_number}: {e}") # Display error

            # ...
    ```

## `Monitor`: Your Performance Black Box

While the `AgentLogger` shows you *what* the agent is doing, the `Monitor` tracks *how well* it's doing it in terms of performance.

**How It's Used (Automatic!)**

The `MultiStepAgent` also creates a `Monitor` instance (`self.monitor`). The monitor's main job is done via its `update_metrics` method. This method is automatically added to a list of `step_callbacks` in the agent. At the end of every single step, the agent calls all functions in `step_callbacks`, including `self.monitor.update_metrics`.

Inside `update_metrics`, the monitor:
1.  Accesses the `ActionStep` object for the just-completed step from [AgentMemory](04_agentmemory.md).
2.  Reads the `duration` recorded in the `ActionStep`.
3.  Accesses the agent's [Model Interface](02_model_interface.md) (`self.tracked_model`) to get the token counts (`last_input_token_count`, `last_output_token_count`) for the LLM call made during that step (if available).
4.  Updates its internal totals (e.g., `total_input_token_count`).
5.  Uses the `AgentLogger` (passed during initialization) to print these metrics, but typically only at the `DEBUG` log level, so they don't clutter the default `INFO` output.

**Example Output (at `DEBUG` level)**

If you run the agent with `verbosity_level=LogLevel.DEBUG`, you'll see the monitor's output added at the end of each step log:

```console
[...]
INFO     Observations: Paris
DEBUG    [Step 1: Duration 1.52 seconds| Input tokens: 150 | Output tokens: 50]  # <-- Monitor Output

[...]
INFO     Observations: Sunny, 25°C
DEBUG    [Step 2: Duration 1.81 seconds| Input tokens: 210 | Output tokens: 105] # <-- Monitor Output

[...]
INFO     [bold #d4b702]Final answer:[/bold #d4b702] The capital of France is Paris, ...
DEBUG    [Step 3: Duration 1.25 seconds| Input tokens: 280 | Output tokens: 170] # <-- Monitor Output
```

**Code Glimpse (`monitoring.py` and `agents.py`)**

*   **`Monitor` Class:** Tracks metrics and logs them.

    ```python
    # --- File: monitoring.py (Simplified Monitor) ---
    from .memory import ActionStep # Needs access to step data
    from .models import Model # Needs access to model token counts
    from .monitoring import AgentLogger, LogLevel # Uses the logger to print

    class Monitor:
        def __init__(self, tracked_model: Model, logger: AgentLogger):
            self.step_durations = []
            self.tracked_model = tracked_model # Reference to the agent's model
            self.logger = logger # Uses the logger to output metrics
            self.total_input_token_count = 0
            self.total_output_token_count = 0
            # ... potentially other metrics ...

        def reset(self):
            """Resets metrics for a new run."""
            self.step_durations = []
            self.total_input_token_count = 0
            self.total_output_token_count = 0

        def update_metrics(self, step_log: ActionStep):
            """Callback function called after each step."""
            # 1. Get duration from the step log
            step_duration = step_log.duration
            self.step_durations.append(step_duration)

            console_outputs = f"[Step {len(self.step_durations)}: Duration {step_duration:.2f} seconds"

            # 2. Get token counts from the model (if available)
            input_tokens = getattr(self.tracked_model, "last_input_token_count", None)
            output_tokens = getattr(self.tracked_model, "last_output_token_count", None)

            if input_tokens is not None and output_tokens is not None:
                self.total_input_token_count += input_tokens
                self.total_output_token_count += output_tokens
                # 4. Format metrics string
                console_outputs += (
                    f"| Input tokens: {self.total_input_token_count:,}"
                    f" | Output tokens: {self.total_output_token_count:,}"
                )
            console_outputs += "]"

            # 5. Log metrics using the logger (at DEBUG level)
            self.logger.log(console_outputs, level=LogLevel.DEBUG) # Note: logs at DEBUG

        # ... methods to get totals, averages etc. ...
    ```

*   **Agent Setting Up the Monitor:**

    ```python
    # --- File: agents.py (Simplified Agent setup for Monitor) ---
    from .monitoring import Monitor
    from .memory import ActionStep

    class MultiStepAgent:
        def __init__(self, ..., model: Model, step_callbacks: Optional[List[Callable]] = None):
            # ... setup logger ...
            self.model = model # Store the model
            self.monitor = Monitor(self.model, self.logger) # Create Monitor

            # Add monitor's update method to callbacks
            self.step_callbacks = step_callbacks if step_callbacks is not None else []
            self.step_callbacks.append(self.monitor.update_metrics)
            # ...

        def _finalize_step(self, memory_step: ActionStep, step_start_time: float):
            """Called at the very end of each step."""
            memory_step.end_time = time.time()
            memory_step.duration = memory_step.end_time - step_start_time

            # Call all registered callbacks, including monitor.update_metrics
            for callback in self.step_callbacks:
                 # Pass the completed step data to the callback
                 callback(memory_step)
            # ...

        def run(self, ..., reset: bool = True):
             # ...
             if reset:
                 self.memory.reset()
                 self.monitor.reset() # Reset monitor metrics on new run
             # ...
    ```

## Conclusion

The `AgentLogger` and `Monitor` are your essential tools for observing and understanding your `SmolaAgents`.

*   **`AgentLogger`** acts as the real-time dashboard, giving you formatted, colorful console output of the agent's steps, thoughts, actions, and errors, crucial for debugging and following along.
*   **`Monitor`** acts as the performance black box, tracking metrics like step duration and token usage, which are logged (usually at the `DEBUG` level) and useful for analysis and optimization.

You've learned:

*   Why visibility into agent execution is critical.
*   The roles of `AgentLogger` (dashboard) and `Monitor` (black box).
*   How they are automatically used by `MultiStepAgent`.
*   How `AgentLogger` provides readable, step-by-step output using `rich`.
*   How `Monitor` tracks performance metrics via step callbacks.
*   How to control log verbosity using `LogLevel`.

With these tools, you're no longer flying blind! You can confidently run your agents, watch them work, understand their performance, and diagnose issues when they arise.

This concludes our introductory tour of the core concepts in `SmolaAgents`. We hope these chapters have given you a solid foundation to start building your own intelligent agents. Happy coding!

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)