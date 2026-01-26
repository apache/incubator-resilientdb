---
layout: default
title: "Crawl4AI"
nav_order: 7
has_children: true
---

# Tutorial: Crawl4AI

> This tutorial is AI-generated! To learn more, check out [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)

`Crawl4AI`<sup>[View Repo](https://github.com/unclecode/crawl4ai/tree/9c58e4ce2ee025debd3f36bf213330bd72b90e46/crawl4ai)</sup> is a flexible Python library for *asynchronously crawling websites* and *extracting structured content*, specifically designed for **AI use cases**.
You primarily interact with the `AsyncWebCrawler`, which acts as the main coordinator. You provide it with URLs and a `CrawlerRunConfig` detailing *how* to crawl (e.g., using specific strategies for fetching, scraping, filtering, and extraction).
It can handle single pages or multiple URLs concurrently using a `BaseDispatcher`, optionally crawl deeper by following links via `DeepCrawlStrategy`, manage `CacheMode`, and apply `RelevantContentFilter` before finally returning a `CrawlResult` containing all the gathered data.

```mermaid
flowchart TD
    A0["AsyncWebCrawler"]
    A1["CrawlerRunConfig"]
    A2["AsyncCrawlerStrategy"]
    A3["ContentScrapingStrategy"]
    A4["ExtractionStrategy"]
    A5["CrawlResult"]
    A6["BaseDispatcher"]
    A7["DeepCrawlStrategy"]
    A8["CacheContext / CacheMode"]
    A9["RelevantContentFilter"]
    A0 -- "Configured by" --> A1
    A0 -- "Uses Fetching Strategy" --> A2
    A0 -- "Uses Scraping Strategy" --> A3
    A0 -- "Uses Extraction Strategy" --> A4
    A0 -- "Produces" --> A5
    A0 -- "Uses Dispatcher for `arun_m..." --> A6
    A0 -- "Uses Caching Logic" --> A8
    A6 -- "Calls Crawler's `arun`" --> A0
    A1 -- "Specifies Deep Crawl Strategy" --> A7
    A7 -- "Processes Links from" --> A5
    A3 -- "Provides Cleaned HTML to" --> A9
    A1 -- "Specifies Content Filter" --> A9
```