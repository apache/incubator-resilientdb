---
layout: default
title: "LevelDB"
nav_order: 14
has_children: true
---

# Tutorial: LevelDB

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

LevelDB<sup>[View Repo](https://github.com/google/leveldb/tree/main/db)</sup> is a fast *key-value storage library* written at Google.
Think of it like a simple database where you store pieces of data (values) associated with unique names (keys).
It's designed to be **very fast** for both writing new data and reading existing data, and it reliably stores everything on **disk**.
It uses a *log-structured merge-tree (LSM-tree)* design to achieve high write performance and manages data in sorted files (*SSTables*) across different levels for efficient reads and space management.

```mermaid
flowchart TD
    A0["DBImpl"]
    A1["MemTable"]
    A2["Table / SSTable & TableCache"]
    A3["Version & VersionSet"]
    A4["Write-Ahead Log (WAL) & LogWriter/LogReader"]
    A5["Iterator"]
    A6["WriteBatch"]
    A7["Compaction"]
    A8["InternalKey & DBFormat"]
    A0 -- "Manages active/immutable" --> A1
    A0 -- "Uses Cache for reads" --> A2
    A0 -- "Manages DB state" --> A3
    A0 -- "Writes to Log" --> A4
    A0 -- "Applies Batches" --> A6
    A0 -- "Triggers/Runs Compaction" --> A7
    A1 -- "Provides Iterator" --> A5
    A1 -- "Stores Keys Using" --> A8
    A2 -- "Provides Iterator via Cache" --> A5
    A3 -- "References SSTables" --> A2
    A3 -- "Picks Files For" --> A7
    A4 -- "Recovers MemTable From" --> A1
    A4 -- "Contains Batch Data" --> A6
    A5 -- "Parses/Hides InternalKey" --> A8
    A6 -- "Inserts Into" --> A1
    A7 -- "Builds SSTables" --> A2
    A7 -- "Updates Versions Via Edit" --> A3
    A7 -- "Uses Iterator for Merging" --> A5
```