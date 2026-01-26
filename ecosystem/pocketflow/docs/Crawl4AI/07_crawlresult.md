---
layout: default
title: "CrawlResult"
parent: "Crawl4AI"
nav_order: 7
---

# Chapter 7: Understanding the Results - CrawlResult

In the previous chapter, [Chapter 6: Getting Specific Data - ExtractionStrategy](06_extractionstrategy.md), we learned how to teach Crawl4AI to act like an analyst, extracting specific, structured data points from a webpage using an `ExtractionStrategy`. We've seen how Crawl4AI can fetch pages, clean them, filter them, and even extract precise information.

But after all that work, where does all the gathered information go? When you ask the `AsyncWebCrawler` to crawl a URL using `arun()`, what do you actually get back?

## What Problem Does `CrawlResult` Solve?

Imagine you sent a research assistant to the library (a website) with a set of instructions: "Find this book (URL), make a clean copy of the relevant chapter (clean HTML/Markdown), list all the cited references (links), take photos of the illustrations (media), find the author and publication date (metadata), and maybe extract specific quotes (structured data)."

When the assistant returns, they wouldn't just hand you a single piece of paper. They'd likely give you a folder containing everything you asked for: the clean copy, the list of references, the photos, the metadata notes, and the extracted quotes, all neatly organized. They might also include a note if they encountered any problems (errors).

`CrawlResult` is exactly this **final report folder** or **delivery package**. It's a single object that neatly contains *all* the information Crawl4AI gathered and processed for a specific URL during a crawl operation. Instead of getting lots of separate pieces of data back, you get one convenient container.

## What is `CrawlResult`?

`CrawlResult` is a Python object (specifically, a Pydantic model, which is like a super-powered dictionary) that acts as a data container. It holds the results of a single crawl task performed by `AsyncWebCrawler.arun()` or one of the results from `arun_many()`.

Think of it as a toolbox filled with different tools and information related to the crawled page.

**Key Information Stored in `CrawlResult`:**

*   **`url` (string):** The original URL that was requested.
*   **`success` (boolean):** Did the crawl complete without critical errors? `True` if successful, `False` otherwise. **Always check this first!**
*   **`html` (string):** The raw, original HTML source code fetched from the page.
*   **`cleaned_html` (string):** The HTML after initial cleaning by the [ContentScrapingStrategy](04_contentscrapingstrategy.md) (e.g., scripts, styles removed).
*   **`markdown` (object):** An object containing different Markdown representations of the content.
    *   `markdown.raw_markdown`: Basic Markdown generated from `cleaned_html`.
    *   `markdown.fit_markdown`: Markdown generated *only* from content deemed relevant by a [RelevantContentFilter](05_relevantcontentfilter.md) (if one was used). Might be empty if no filter was applied.
    *   *(Other fields like `markdown_with_citations` might exist)*
*   **`extracted_content` (string):** If you used an [ExtractionStrategy](06_extractionstrategy.md), this holds the extracted structured data, usually formatted as a JSON string. `None` if no extraction was performed or nothing was found.
*   **`metadata` (dictionary):** Information extracted from the page's metadata tags, like the page title (`metadata['title']`), description, keywords, etc.
*   **`links` (object):** Contains lists of links found on the page.
    *   `links.internal`: List of links pointing to the same website.
    *   `links.external`: List of links pointing to other websites.
*   **`media` (object):** Contains lists of media items found.
    *   `media.images`: List of images (`<img>` tags).
    *   `media.videos`: List of videos (`<video>` tags).
    *   *(Other media types might be included)*
*   **`screenshot` (string):** If you requested a screenshot (`screenshot=True` in `CrawlerRunConfig`), this holds the file path to the saved image. `None` otherwise.
*   **`pdf` (bytes):** If you requested a PDF (`pdf=True` in `CrawlerRunConfig`), this holds the PDF data as bytes. `None` otherwise. (Note: Previously might have been a path, now often bytes).
*   **`error_message` (string):** If `success` is `False`, this field usually contains details about what went wrong.
*   **`status_code` (integer):** The HTTP status code received from the server (e.g., 200 for OK, 404 for Not Found).
*   **`response_headers` (dictionary):** The HTTP response headers sent by the server.
*   **`redirected_url` (string):** If the original URL redirected, this shows the final URL the crawler landed on.

## Accessing the `CrawlResult`

You get a `CrawlResult` object back every time you `await` a call to `crawler.arun()`:

```python
# chapter7_example_1.py
import asyncio
from crawl4ai import AsyncWebCrawler

async def main():
    async with AsyncWebCrawler() as crawler:
        url = "https://httpbin.org/html"
        print(f"Crawling {url}...")

        # The 'arun' method returns a CrawlResult object
        result: CrawlResult = await crawler.arun(url=url) # Type hint optional

        print("Crawl finished!")
        # Now 'result' holds all the information
        print(f"Result object type: {type(result)}")

if __name__ == "__main__":
    asyncio.run(main())
```

**Explanation:**

1.  We call `crawler.arun(url=url)`.
2.  The `await` keyword pauses execution until the crawl is complete.
3.  The value returned by `arun` is assigned to the `result` variable.
4.  This `result` variable is our `CrawlResult` object.

If you use `crawler.arun_many()`, it returns a list where each item is a `CrawlResult` object for one of the requested URLs (or an async generator if `stream=True`).

## Exploring the Attributes: Using the Toolbox

Once you have the `result` object, you can access its attributes using dot notation (e.g., `result.success`, `result.markdown`).

**1. Checking for Success (Most Important!)**

Before you try to use any data, always check if the crawl was successful:

```python
# chapter7_example_2.py
import asyncio
from crawl4ai import AsyncWebCrawler, CrawlResult # Import CrawlResult for type hint

async def main():
    async with AsyncWebCrawler() as crawler:
        url = "https://httpbin.org/html" # A working URL
        # url = "https://httpbin.org/status/404" # Try this URL to see failure
        result: CrawlResult = await crawler.arun(url=url)

        # --- ALWAYS CHECK 'success' FIRST! ---
        if result.success:
            print(f"✅ Successfully crawled: {result.url}")
            # Now it's safe to access other attributes
            print(f"   Page Title: {result.metadata.get('title', 'N/A')}")
        else:
            print(f"❌ Failed to crawl: {result.url}")
            print(f"   Error: {result.error_message}")
            print(f"   Status Code: {result.status_code}")

if __name__ == "__main__":
    asyncio.run(main())
```

**Explanation:**

*   We use an `if result.success:` block.
*   If `True`, we proceed to access other data like `result.metadata`.
*   If `False`, we print the `result.error_message` and `result.status_code` to understand why it failed.

**2. Accessing Content (HTML, Markdown)**

```python
# chapter7_example_3.py
import asyncio
from crawl4ai import AsyncWebCrawler, CrawlResult

async def main():
    async with AsyncWebCrawler() as crawler:
        url = "https://httpbin.org/html"
        result: CrawlResult = await crawler.arun(url=url)

        if result.success:
            print("--- Content ---")
            # Print the first 150 chars of raw HTML
            print(f"Raw HTML snippet: {result.html[:150]}...")

            # Access the raw markdown
            if result.markdown: # Check if markdown object exists
                 print(f"Markdown snippet: {result.markdown.raw_markdown[:150]}...")
            else:
                 print("Markdown not generated.")
        else:
            print(f"Crawl failed: {result.error_message}")

if __name__ == "__main__":
    asyncio.run(main())
```

**Explanation:**

*   We access `result.html` for the original HTML.
*   We access `result.markdown.raw_markdown` for the main Markdown content. Note the two dots: `result.markdown` gives the `MarkdownGenerationResult` object, and `.raw_markdown` accesses the specific string within it. We also check `if result.markdown:` first, just in case markdown generation failed for some reason.

**3. Getting Metadata, Links, and Media**

```python
# chapter7_example_4.py
import asyncio
from crawl4ai import AsyncWebCrawler, CrawlResult

async def main():
    async with AsyncWebCrawler() as crawler:
        url = "https://httpbin.org/links/10/0" # A page with links
        result: CrawlResult = await crawler.arun(url=url)

        if result.success:
            print("--- Metadata & Links ---")
            print(f"Title: {result.metadata.get('title', 'N/A')}")
            print(f"Found {len(result.links.internal)} internal links.")
            print(f"Found {len(result.links.external)} external links.")
            if result.links.internal:
                print(f"  First internal link text: '{result.links.internal[0].text}'")
            # Similarly access result.media.images etc.
        else:
            print(f"Crawl failed: {result.error_message}")

if __name__ == "__main__":
    asyncio.run(main())
```

**Explanation:**

*   `result.metadata` is a dictionary; use `.get()` for safe access.
*   `result.links` and `result.media` are objects containing lists (`internal`, `external`, `images`, etc.). We can check their lengths (`len()`) and access individual items by index (e.g., `[0]`).

**4. Checking for Extracted Data, Screenshots, PDFs**

```python
# chapter7_example_5.py
import asyncio
import json
from crawl4ai import (
    AsyncWebCrawler, CrawlResult, CrawlerRunConfig,
    JsonCssExtractionStrategy # Example extractor
)

async def main():
    # Define a simple extraction strategy (from Chapter 6)
    schema = {"baseSelector": "body", "fields": [{"name": "heading", "selector": "h1", "type": "text"}]}
    extractor = JsonCssExtractionStrategy(schema=schema)

    # Configure the run to extract and take a screenshot
    config = CrawlerRunConfig(
        extraction_strategy=extractor,
        screenshot=True
    )

    async with AsyncWebCrawler() as crawler:
        url = "https://httpbin.org/html"
        result: CrawlResult = await crawler.arun(url=url, config=config)

        if result.success:
            print("--- Extracted Data & Media ---")
            # Check if structured data was extracted
            if result.extracted_content:
                print("Extracted Data found:")
                data = json.loads(result.extracted_content) # Parse the JSON string
                print(json.dumps(data, indent=2))
            else:
                print("No structured data extracted.")

            # Check if a screenshot was taken
            if result.screenshot:
                print(f"Screenshot saved to: {result.screenshot}")
            else:
                print("Screenshot not taken.")

            # Check for PDF (would be bytes if requested and successful)
            if result.pdf:
                 print(f"PDF data captured ({len(result.pdf)} bytes).")
            else:
                 print("PDF not generated.")
        else:
            print(f"Crawl failed: {result.error_message}")

if __name__ == "__main__":
    asyncio.run(main())
```

**Explanation:**

*   We check if `result.extracted_content` is not `None` or empty before trying to parse it as JSON.
*   We check if `result.screenshot` is not `None` to see if the file path exists.
*   We check if `result.pdf` is not `None` to see if the PDF data (bytes) was captured.

## How is `CrawlResult` Created? (Under the Hood)

You don't interact with the `CrawlResult` constructor directly. The `AsyncWebCrawler` creates it for you at the very end of the `arun` process, typically inside its internal `aprocess_html` method (or just before returning if fetching from cache).

Here's a simplified sequence:

1.  **Fetch:** `AsyncWebCrawler` calls the [AsyncCrawlerStrategy](01_asynccrawlerstrategy.md) to get the raw `html`, `status_code`, `response_headers`, etc.
2.  **Scrape:** It passes the `html` to the [ContentScrapingStrategy](04_contentscrapingstrategy.md) to get `cleaned_html`, `links`, `media`, `metadata`.
3.  **Markdown:** It generates Markdown using the configured generator, possibly involving a [RelevantContentFilter](05_relevantcontentfilter.md), resulting in a `MarkdownGenerationResult` object.
4.  **Extract (Optional):** If an [ExtractionStrategy](06_extractionstrategy.md) is configured, it runs it on the appropriate content (HTML or Markdown) to get `extracted_content`.
5.  **Screenshot/PDF (Optional):** If requested, the fetching strategy captures the `screenshot` path or `pdf` data.
6.  **Package:** `AsyncWebCrawler` gathers all these pieces (`url`, `html`, `cleaned_html`, the markdown object, `links`, `media`, `metadata`, `extracted_content`, `screenshot`, `pdf`, `success` status, `error_message`, etc.).
7.  **Instantiate:** It creates the `CrawlResult` object, passing all the gathered data into its constructor.
8.  **Return:** It returns this fully populated `CrawlResult` object to your code.

## Code Glimpse (`models.py`)

The `CrawlResult` is defined in the `crawl4ai/models.py` file. It uses Pydantic, a library that helps define data structures with type hints and validation. Here's a simplified view:

```python
# Simplified from crawl4ai/models.py
from pydantic import BaseModel, HttpUrl
from typing import List, Dict, Optional, Any

# Other related models (simplified)
class MarkdownGenerationResult(BaseModel):
    raw_markdown: str
    fit_markdown: Optional[str] = None
    # ... other markdown fields ...

class Links(BaseModel):
    internal: List[Dict] = []
    external: List[Dict] = []

class Media(BaseModel):
    images: List[Dict] = []
    videos: List[Dict] = []

# The main CrawlResult model
class CrawlResult(BaseModel):
    url: str
    html: str
    success: bool
    cleaned_html: Optional[str] = None
    media: Media = Media() # Use the Media model
    links: Links = Links() # Use the Links model
    screenshot: Optional[str] = None
    pdf: Optional[bytes] = None
    # Uses a private attribute and property for markdown for compatibility
    _markdown: Optional[MarkdownGenerationResult] = None # Actual storage
    extracted_content: Optional[str] = None # JSON string
    metadata: Optional[Dict[str, Any]] = None
    error_message: Optional[str] = None
    status_code: Optional[int] = None
    response_headers: Optional[Dict[str, str]] = None
    redirected_url: Optional[str] = None
    # ... other fields like session_id, ssl_certificate ...

    # Custom property to access markdown data
    @property
    def markdown(self) -> Optional[MarkdownGenerationResult]:
        return self._markdown

    # Configuration for Pydantic
    class Config:
        arbitrary_types_allowed = True

    # Custom init and model_dump might exist for backward compatibility handling
    # ... (omitted for simplicity) ...
```

**Explanation:**

*   It's defined as a `class CrawlResult(BaseModel):`.
*   Each attribute (like `url`, `html`, `success`) is defined with a type hint (like `str`, `bool`, `Optional[str]`). `Optional[str]` means the field can be a string or `None`.
*   Some attributes are themselves complex objects defined by other Pydantic models (like `media: Media`, `links: Links`).
*   The `markdown` field uses a common pattern (property wrapping a private attribute) to provide the `MarkdownGenerationResult` object while maintaining some backward compatibility. You access it simply as `result.markdown`.

## Conclusion

You've now met the `CrawlResult` object – the final, comprehensive report delivered by Crawl4AI after processing a URL.

*   It acts as a **container** holding all gathered information (HTML, Markdown, metadata, links, media, extracted data, errors, etc.).
*   It's the **return value** of `AsyncWebCrawler.arun()` and `arun_many()`.
*   The most crucial attribute is **`success` (boolean)**, which you should always check first.
*   You can easily **access** all the different pieces of information using dot notation (e.g., `result.metadata['title']`, `result.markdown.raw_markdown`, `result.links.external`).

Understanding the `CrawlResult` is key to effectively using the information Crawl4AI provides.

So far, we've focused on crawling single pages or lists of specific URLs. But what if you want to start at one page and automatically discover and crawl linked pages, exploring a website more deeply?

**Next:** Let's explore how to perform multi-page crawls with [Chapter 8: Exploring Websites - DeepCrawlStrategy](08_deepcrawlstrategy.md).

---

Generated by [AI Codebase Knowledge Builder](https://github.com/The-Pocket/Tutorial-Codebase-Knowledge)