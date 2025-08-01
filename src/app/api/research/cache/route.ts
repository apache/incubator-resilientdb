import { NextRequest, NextResponse } from "next/server";
import { simpleDocumentService } from "../../../../lib/simple-document-service";

/**
 * Cache management API endpoints
 * GET: Get cache statistics
 * DELETE: Clear cache or remove specific document
 */

export async function GET() {
  try {
    const stats = await simpleDocumentService.getCacheStats();
    
    return NextResponse.json({
      success: true,
      data: {
        documentCount: stats.documentCount,
        totalChunks: stats.totalChunks,
        oldestDocument: stats.oldestDocument,
        newestDocument: stats.newestDocument
      }
    });

  } catch (error) {
    console.error("Cache stats error:", error);
    return NextResponse.json(
      { 
        success: false, 
        error: "Failed to get cache statistics",
        details: error instanceof Error ? error.message : String(error)
      },
      { status: 500 }
    );
  }
}

export async function DELETE(request: NextRequest) {
  try {
    const { searchParams } = new URL(request.url);
    const documentPath = searchParams.get('document');

    if (documentPath) {
      // Remove specific document from cache
      await simpleDocumentService.removeCachedDocument(documentPath);
      return NextResponse.json({
        success: true,
        message: `Removed cached document: ${documentPath}`
      });
    } else {
      // Clear entire cache
      await simpleDocumentService.clearCache();
      return NextResponse.json({
        success: true,
        message: "Cache cleared successfully"
      });
    }

  } catch (error) {
    console.error("Cache management error:", error);
    return NextResponse.json(
      { 
        success: false, 
        error: "Failed to manage cache",
        details: error instanceof Error ? error.message : String(error)
      },
      { status: 500 }
    );
  }
}