/**
 * Simple in-memory cache for LLM responses
 * Reduces API calls by caching responses for identical queries
 */

interface CacheEntry<T> {
  data: T;
  timestamp: number;
  expiresAt: number;
}

export class SimpleCache<T> {
  private cache: Map<string, CacheEntry<T>>;
  private defaultTTL: number; // Time to live in milliseconds

  constructor(defaultTTL: number = 3600000) { // Default: 1 hour
    this.cache = new Map();
    this.defaultTTL = defaultTTL;
  }

  /**
   * Generate cache key from query and options
   */
  private generateKey(query: string, options?: Record<string, unknown>): string {
    const optionsStr = options ? JSON.stringify(options) : '';
    return `${query}:${optionsStr}`;
  }

  /**
   * Get cached value if available and not expired
   */
  get(query: string, options?: Record<string, unknown>): T | null {
    const key = this.generateKey(query, options);
    const entry = this.cache.get(key);

    if (!entry) {
      return null;
    }

    // Check if expired
    if (Date.now() > entry.expiresAt) {
      this.cache.delete(key);
      return null;
    }

    return entry.data;
  }

  /**
   * Set cache value with TTL
   */
  set(query: string, data: T, options?: Record<string, unknown>, ttl?: number): void {
    const key = this.generateKey(query, options);
    const now = Date.now();
    const expiresAt = now + (ttl || this.defaultTTL);

    this.cache.set(key, {
      data,
      timestamp: now,
      expiresAt,
    });
  }

  /**
   * Clear expired entries
   */
  clearExpired(): void {
    const now = Date.now();
    for (const [key, entry] of this.cache.entries()) {
      if (now > entry.expiresAt) {
        this.cache.delete(key);
      }
    }
  }

  /**
   * Clear all cache
   */
  clear(): void {
    this.cache.clear();
  }

  /**
   * Get cache size
   */
  size(): number {
    return this.cache.size;
  }
}

