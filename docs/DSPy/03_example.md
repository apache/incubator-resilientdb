---
layout: default
title: "Example"
parent: "DSPy"
nav_order: 3
---

# Chapter 3: Example - Your Data Points

In [Chapter 2: Signature](02_signature.md), we learned how to define the *task* for a DSPy module using `Signatures` – specifying the inputs, outputs, and instructions. It's like writing a recipe card.

But sometimes, just giving instructions isn't enough. Imagine teaching someone to translate by just giving the rule "Translate English to French". They might struggle! It often helps to show them a few *examples* of correct translations.

This is where **`dspy.Example`** comes in! It's how you represent individual data points or examples within DSPy.

Think of a `dspy.Example` as:

*   **A Single Row:** Like one row in a spreadsheet or database table.
*   **A Flashcard:** Holding a specific question and its answer, or an input and its desired output.
*   **A Test Case:** A concrete instance of the task defined by your `Signature`.

In this chapter, we'll learn:

*   What a `dspy.Example` is and how it stores data.
*   How to create `Example` objects.
*   Why `Example`s are essential for few-shot learning, training, and evaluation.
*   How to mark specific fields as inputs using `.with_inputs()`.

Let's dive into representing our data!

## What is a `dspy.Example`?

A `dspy.Example` is a fundamental data structure in DSPy designed to hold the information for a single instance of your task. It essentially acts like a flexible container (similar to a Python dictionary) where you store key-value pairs.

Crucially, the **keys** in your `Example` should generally match the **field names** you defined in your [Signature](02_signature.md).

Let's revisit our `TranslateToFrench` signature from Chapter 2:

```python
# From Chapter 2
import dspy
from dspy.signatures.field import InputField, OutputField

class TranslateToFrench(dspy.Signature):
    """Translates English text to French."""
    english_sentence = dspy.InputField(desc="The original sentence in English")
    french_sentence = dspy.OutputField(desc="The translated sentence in French")
```

This signature has two fields: `english_sentence` (input) and `french_sentence` (output).

An `Example` representing one instance of this task would need to contain values for these keys.

## Creating an Example

Creating a `dspy.Example` is straightforward. You can initialize it with keyword arguments, where the argument names match the fields you care about (usually your Signature fields).

```python
import dspy

# Create an example for our translation task
example1 = dspy.Example(
    english_sentence="Hello, world!",
    french_sentence="Bonjour le monde!"
)

# You can access the values like attributes
print(f"English: {example1.english_sentence}")
print(f"French: {example1.french_sentence}")
```

**Output:**

```
English: Hello, world!
French: Bonjour le monde!
```

See? `example1` now holds one complete data point for our translation task. It bundles the input (`english_sentence`) and the corresponding desired output (`french_sentence`) together.

You can also create examples from dictionaries:

```python
data_dict = {
    "english_sentence": "How are you?",
    "french_sentence": "Comment ça va?"
}
example2 = dspy.Example(data_dict)

print(f"Example 2 English: {example2.english_sentence}")
```

**Output:**

```
Example 2 English: How are you?
```

## Why Use Examples? The Three Main Roles

`Example` objects are the standard way DSPy handles data, and they are used in three critical ways:

1.  **Few-Shot Demonstrations:** When using modules like `dspy.Predict` (which we'll see in [Chapter 4: Predict](04_predict.md)), you can provide a few `Example` objects directly in the prompt sent to the Language Model (LM). This shows the LM *exactly* how to perform the task, often leading to much better results than instructions alone. It's like showing the chef pictures of the final dish alongside the recipe.

2.  **Training Data:** When you want to *optimize* your DSPy program (e.g., automatically find the best prompts or few-shot examples), you use **Teleprompters** ([Chapter 8: Teleprompter / Optimizer](08_teleprompter___optimizer.md)). Teleprompters require a training set, which is simply a list of `dspy.Example` objects representing the tasks you want your program to learn to do well.

3.  **Evaluation Data:** How do you know if your DSPy program is working correctly? You test it on a dataset! The `dspy.evaluate` module ([Chapter 7: Evaluate](07_evaluate.md)) takes a list of `dspy.Example` objects (your test set or development set) and measures your program's performance against the expected outputs (labels) in those examples.

In all these cases, `dspy.Example` provides a consistent way to package and manage your data points.

## Marking Inputs: `.with_inputs()`

Often, especially during training and evaluation, DSPy needs to know which fields in your `Example` represent the *inputs* to your program and which represent the *outputs* or *labels* (the ground truth answers).

The `.with_inputs()` method allows you to explicitly mark certain keys as input fields. This method returns a *new* `Example` object with this input information attached, leaving the original unchanged.

Let's mark `english_sentence` as the input for our `example1`:

```python
# Our original example
example1 = dspy.Example(
    english_sentence="Hello, world!",
    french_sentence="Bonjour le monde!"
)

# Mark 'english_sentence' as the input field
input_marked_example = example1.with_inputs("english_sentence")

# Let's check the inputs and labels (non-inputs)
print(f"Inputs: {input_marked_example.inputs()}")
print(f"Labels: {input_marked_example.labels()}")
```

**Output:**

```
Inputs: Example({'english_sentence': 'Hello, world!'}) (input_keys={'english_sentence'})
Labels: Example({'french_sentence': 'Bonjour le monde!'}) (input_keys=set())
```

Notice:
*   `.with_inputs("english_sentence")` didn't change `example1`. It created `input_marked_example`.
*   `input_marked_example.inputs()` returns a new `Example` containing only the fields marked as inputs.
*   `input_marked_example.labels()` returns a new `Example` containing the remaining fields (the outputs/labels).

This distinction is vital for evaluation (comparing predictions against labels) and optimization (knowing what the program receives vs. what it should produce). Datasets loaded within DSPy often automatically handle marking inputs for you based on common conventions.

## How It Works Under the Hood (A Peek)

The `dspy.Example` object is fundamentally quite simple. It's designed to behave much like a Python dictionary but with some added conveniences like attribute-style access (`example.field`) and the special `.with_inputs()` method.

1.  **Storage:** Internally, an `Example` uses a dictionary (often named `_store`) to hold all the key-value pairs you provide.
    ```python
    # Conceptual internal structure
    example = dspy.Example(question="What is DSPy?", answer="A framework...")
    # example._store == {'question': 'What is DSPy?', 'answer': 'A framework...'}
    ```
2.  **Attribute Access:** When you access `example.question`, Python's magic methods (`__getattr__`) look up `'question'` in the internal `_store`. Similarly, setting `example.new_field = value` uses `__setattr__` to update the `_store`.
3.  **`.with_inputs()`:** This method creates a *copy* of the current `Example`'s `_store`. It then stores the provided input keys (like `{'english_sentence'}`) in a separate internal attribute (like `_input_keys`) on the *new* copied object. It doesn't modify the original `Example`.
4.  **`.inputs()` and `.labels()`:** These methods check the `_input_keys` attribute. `.inputs()` creates a new `Example` containing only the key-value pairs whose keys are *in* `_input_keys`. `.labels()` creates a new `Example` containing the key-value pairs whose keys are *not* in `_input_keys`.

Let's look at a simplified view of the code from `dspy/primitives/example.py`:

```python
# Simplified view from dspy/primitives/example.py

class Example:
    def __init__(self, base=None, **kwargs):
        self._store = {}  # The internal dictionary
        self._input_keys = None # Stores the input keys after with_inputs()

        # Simplified: Copy from base or dictionary if provided
        if base and isinstance(base, dict): self._store = base.copy()
        # Simplified: Update with keyword arguments
        self._store.update(kwargs)

    # Allows accessing self.key like dictionary lookup self._store[key]
    def __getattr__(self, key):
        if key in self._store: return self._store[key]
        raise AttributeError(f"No attribute '{key}'")

    # Allows setting self.key like dictionary assignment self._store[key] = value
    def __setattr__(self, key, value):
        if key.startswith("_"): super().__setattr__(key, value) # Handle internal attributes
        else: self._store[key] = value

    # Allows dictionary-style access example[key]
    def __getitem__(self, key): return self._store[key]

    # Creates a *copy* and marks input keys on the copy.
    def with_inputs(self, *keys):
        copied = self.copy() # Make a shallow copy
        copied._input_keys = set(keys) # Store the input keys on the copy
        return copied

    # Returns a new Example containing only input fields.
    def inputs(self):
        if self._input_keys is None: raise ValueError("Inputs not set.")
        # Create a dict with only input keys
        input_dict = {k: v for k, v in self._store.items() if k in self._input_keys}
        # Return a new Example wrapping this dict
        return type(self)(base=input_dict).with_inputs(*self._input_keys)

    # Returns a new Example containing only non-input fields (labels).
    def labels(self):
        input_keys = self.inputs().keys() if self._input_keys else set()
        # Create a dict with only non-input keys
        label_dict = {k: v for k, v in self._store.items() if k not in input_keys}
        # Return a new Example wrapping this dict
        return type(self)(base=label_dict)

    # Helper to create a copy
    def copy(self, **kwargs):
        return type(self)(base=self, **kwargs)

    # ... other helpful methods like keys(), values(), items(), etc. ...
```

The key idea is that `dspy.Example` provides a convenient and standardized wrapper around your data points, making it easy to use them for few-shot examples, training, and evaluation, while also allowing you to specify which parts are inputs versus labels.

## Conclusion

You've now mastered `dspy.Example`, the way DSPy represents individual data points!

*   An `Example` holds key-value pairs, like a **row in a spreadsheet** or a **flashcard**.
*   Its keys typically correspond to the fields defined in a [Signature](02_signature.md).
*   `Example`s are essential for providing **few-shot demonstrations**, **training data** for optimizers ([Teleprompter / Optimizer](08_teleprompter___optimizer.md)), and **evaluation data** for testing ([Evaluate](07_evaluate.md)).
*   The `.with_inputs()` method lets you mark which fields are inputs, crucial for distinguishing inputs from labels.

Now that we have `Signatures` to define *what* task to do, and `Examples` to hold the *data* for that task, how do we actually get a Language Model to *do* the task based on the signature? That's the job of the `dspy.Predict` module!

**Next:** [Chapter 4: Predict](04_predict.md)

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)