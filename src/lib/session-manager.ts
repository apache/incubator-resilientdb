import { createMemory } from "llamaindex";

export interface SessionMetadata {
  memory: any;
  created: number;
  lastAccessed: number;
}

export const SESSION_CONFIG = {
  MAX_SESSIONS: 100,
  SESSION_TTL_MS: 24 * 60 * 60 * 1000, // 24 hours
  CLEANUP_INTERVAL_MS: 60 * 60 * 1000, // 1 hour
} as const;

class SessionManager {
  private sessionMemories = new Map<string, SessionMetadata>();
  private lastCleanup = 0;

  cleanupSessions() {
    const now = Date.now();
    const expiredSessions: string[] = [];
    
    for (const [sessionId, metadata] of this.sessionMemories.entries()) {
      if (now - metadata.lastAccessed > SESSION_CONFIG.SESSION_TTL_MS) {
        expiredSessions.push(sessionId);
      }
    }
    
    expiredSessions.forEach(sessionId => this.sessionMemories.delete(sessionId));
    
    // If still over limit, remove oldest sessions
    if (this.sessionMemories.size > SESSION_CONFIG.MAX_SESSIONS) {
      const entries = Array.from(this.sessionMemories.entries())
        .sort(([, a], [, b]) => a.lastAccessed - b.lastAccessed);
      
      const toDelete = entries.slice(0, entries.length - SESSION_CONFIG.MAX_SESSIONS);
      toDelete.forEach(([sessionId]) => this.sessionMemories.delete(sessionId));
    }
    
    this.lastCleanup = now;
    console.log(`Session cleanup completed. Active sessions: ${this.sessionMemories.size}`);
  }

  getSessionMemory(sessionId: string) {
    const now = Date.now();
    
    // Run cleanup periodically
    if (now - this.lastCleanup > SESSION_CONFIG.CLEANUP_INTERVAL_MS) {
      this.cleanupSessions();
    }
    
    if (!this.sessionMemories.has(sessionId)) {
      const memory = createMemory({
        tokenLimit: 4000, 
        shortTermTokenLimitRatio: 0.7,
      });
      
      this.sessionMemories.set(sessionId, {
        memory,
        created: now,
        lastAccessed: now,
      });
    } else {
      // Update last accessed time
      const metadata = this.sessionMemories.get(sessionId)!;
      metadata.lastAccessed = now;
    }
    
    return this.sessionMemories.get(sessionId)!.memory;
  }

  clearSession(sessionId: string) {
    this.sessionMemories.delete(sessionId);
  }

  getAllSessions(): Array<{ sessionId: string; metadata: SessionMetadata }> {
    return Array.from(this.sessionMemories.entries()).map(([sessionId, metadata]) => ({
      sessionId,
      metadata
    }));
  }

  getSessionCount(): number {
    return this.sessionMemories.size;
  }

  getSessionStats() {
    const now = Date.now();
    const sessions = this.getAllSessions();
    
    return {
      total: sessions.length,
      maxAllowed: SESSION_CONFIG.MAX_SESSIONS,
      oldestSession: sessions.length > 0 ? Math.min(...sessions.map(s => s.metadata.created)) : null,
      newestSession: sessions.length > 0 ? Math.max(...sessions.map(s => s.metadata.created)) : null,
      lastCleanup: this.lastCleanup,
      nextCleanup: this.lastCleanup + SESSION_CONFIG.CLEANUP_INTERVAL_MS,
      timeUntilNextCleanup: Math.max(0, (this.lastCleanup + SESSION_CONFIG.CLEANUP_INTERVAL_MS) - now)
    };
  }
}

// Export singleton instance
export const sessionManager = new SessionManager();
