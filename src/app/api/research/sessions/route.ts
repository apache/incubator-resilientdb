import { SESSION_CONFIG, sessionManager } from "@/lib/session-manager";
import { NextRequest, NextResponse } from "next/server";

interface SessionInfo {
  sessionId: string;
  created: string;
  lastAccessed: string;
  age: string;
  timeSinceLastAccess: string;
  memoryTokenCount?: number;
}

function formatDuration(ms: number): string {
  const seconds = Math.floor(ms / 1000);
  const minutes = Math.floor(seconds / 60);
  const hours = Math.floor(minutes / 60);
  const days = Math.floor(hours / 24);

  if (days > 0) return `${days}d ${hours % 24}h`;
  if (hours > 0) return `${hours}h ${minutes % 60}m`;
  if (minutes > 0) return `${minutes}m ${seconds % 60}s`;
  return `${seconds}s`;
}

function formatTimestamp(timestamp: number): string {
  return new Date(timestamp).toISOString();
}

export async function GET() {
  try {
    const now = Date.now();
    const sessions = sessionManager.getAllSessions();
    const stats = sessionManager.getSessionStats();

    const sessionInfos: SessionInfo[] = sessions.map(({ sessionId, metadata }) => {
      const age = now - metadata.created;
      const timeSinceLastAccess = now - metadata.lastAccessed;

      return {
        sessionId,
        created: formatTimestamp(metadata.created),
        lastAccessed: formatTimestamp(metadata.lastAccessed),
        age: formatDuration(age),
        timeSinceLastAccess: formatDuration(timeSinceLastAccess),
        // We could add memory token count if the memory object exposes it
      };
    });

    // Sort by most recently accessed
    sessionInfos.sort((a, b) => new Date(b.lastAccessed).getTime() - new Date(a.lastAccessed).getTime());

    return NextResponse.json({
      stats: {
        totalSessions: stats.total,
        maxSessions: stats.maxAllowed,
        sessionTtl: formatDuration(SESSION_CONFIG.SESSION_TTL_MS),
        cleanupInterval: formatDuration(SESSION_CONFIG.CLEANUP_INTERVAL_MS),
        lastCleanup: stats.lastCleanup ? formatTimestamp(stats.lastCleanup) : "Never",
        nextCleanup: stats.nextCleanup ? formatTimestamp(stats.nextCleanup) : "Unknown",
        timeUntilNextCleanup: formatDuration(stats.timeUntilNextCleanup),
        oldestSession: stats.oldestSession ? formatTimestamp(stats.oldestSession) : null,
        newestSession: stats.newestSession ? formatTimestamp(stats.newestSession) : null,
      },
      sessions: sessionInfos,
    });

  } catch (error) {
    console.error("Error retrieving session info:", error);
    return NextResponse.json(
      {
        error: "Failed to retrieve session information",
        details: error instanceof Error ? error.message : String(error),
      },
      { status: 500 }
    );
  }
}

export async function DELETE(req: NextRequest) {
  try {
    const url = new URL(req.url);
    const sessionId = url.searchParams.get('sessionId');

    if (sessionId) {
      // Clear specific session
      sessionManager.clearSession(sessionId);
      return NextResponse.json({
        message: `Session ${sessionId} cleared successfully`
      });
    } else {
      // Force cleanup of expired sessions
      sessionManager.cleanupSessions();
      return NextResponse.json({
        message: "Session cleanup completed"
      });
    }
  } catch (error) {
    console.error("Error clearing session:", error);
    return NextResponse.json(
      {
        error: "Failed to clear session",
        details: error instanceof Error ? error.message : String(error),
      },
      { status: 500 }
    );
  }
}
