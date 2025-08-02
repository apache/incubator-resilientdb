import { NextRequest, NextResponse } from "next/server";
import { documentService } from "../../../../lib/document-service";

export async function GET(req: NextRequest) {
  try {
    const { searchParams } = new URL(req.url);
    const action = searchParams.get('action');

    if (action === 'stats') {
      // Get duplicate statistics
      const stats = await documentService.getDuplicateStats();
      return NextResponse.json({
        success: true,
        stats
      });
    }

    return NextResponse.json(
      { error: "Invalid action. Use ?action=stats" },
      { status: 400 }
    );

  } catch (error) {
    console.error("Error in cleanup API:", error);
    return NextResponse.json(
      {
        error: "Failed to get cleanup information",
        details: error instanceof Error ? error.message : String(error),
      },
      { status: 500 }
    );
  }
}

export async function POST(req: NextRequest) {
  try {
    const { action } = await req.json();

    if (action === 'remove_duplicates') {
      // Remove duplicate documents
      await documentService.removeDuplicateDocuments();
      
      // Get updated statistics
      const stats = await documentService.getDuplicateStats();
      
      return NextResponse.json({
        success: true,
        message: "Duplicate documents removed successfully",
        stats
      });
    }

    if (action === 'clear_cache') {
      // Clear parsed document cache
      await documentService.clearCache();
      
      return NextResponse.json({
        success: true,
        message: "Document cache cleared successfully"
      });
    }

    return NextResponse.json(
      { error: "Invalid action. Use 'remove_duplicates' or 'clear_cache'" },
      { status: 400 }
    );

  } catch (error) {
    console.error("Error in cleanup API:", error);
    return NextResponse.json(
      {
        error: "Failed to perform cleanup action",
        details: error instanceof Error ? error.message : String(error),
      },
      { status: 500 }
    );
  }
}