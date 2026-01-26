---
layout: default
title: "NumPy Core"
nav_order: 16
has_children: true
---

# Tutorial: NumPy Core

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

NumPy Core<sup>[View Repo](https://github.com/numpy/numpy/tree/3b377854e8b1a55f15bda6f1166fe9954828231b/numpy/_core)</sup> provides the powerful **ndarray** object, a *multi-dimensional grid* optimized for numerical computations on large datasets. It uses **dtypes** (data type objects) to precisely define the *kind of data* (like integers or floating-point numbers) stored within an array, ensuring memory efficiency and enabling optimized low-level operations. NumPy also features **ufuncs** (universal functions), which are functions like `add` or `sin` designed to operate *element-wise* on entire arrays very quickly, leveraging compiled code. Together, these components form the foundation for high-performance scientific computing in Python.

```mermaid
flowchart TD
    A0["ndarray (N-dimensional array)"]
    A1["dtype (Data Type Object)"]
    A2["ufunc (Universal Function)"]
    A3["multiarray Module"]
    A4["umath Module"]
    A5["Numeric Types"]
    A6["Array Printing"]
    A7["__array_function__ Protocol / Overrides"]
    A0 -- "Has data type" --> A1
    A2 -- "Operates element-wise on" --> A0
    A3 -- "Provides implementation for" --> A0
    A4 -- "Provides implementation for" --> A2
    A5 -- "Defines scalar types for" --> A1
    A6 -- "Formats for display" --> A0
    A6 -- "Uses for formatting info" --> A1
    A7 -- "Overrides functions from" --> A3
    A7 -- "Overrides functions from" --> A4
    A1 -- "References type hierarchy" --> A5
```