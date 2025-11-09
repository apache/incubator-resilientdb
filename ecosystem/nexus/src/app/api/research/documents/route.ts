// import { readdir, stat } from "fs/promises";
import { NextResponse } from "next/server";
// import { join } from "path";
import { getFilesFromMultipleFolders } from "@/lib/google-drive";

export async function GET() {
  try {
    // Define your folder IDs as an array
    const folderIds = [
      "16yvY-jnMOZ1hMGaMYk0oi86eBbCqCHR2",
      "1BKiGBrrrBMegCBUV1RlpkWBTktNPt4GL"
    ];

    if (!folderIds || folderIds.length === 0) {
      return NextResponse.json({ error: "No folder IDs provided" }, { status: 400 });
    }

    // Get files from multiple folders using batch request
    const filesByFolder = await getFilesFromMultipleFolders(folderIds);

    // Optional: Add summary information
    const summary = {
      totalFolders: folderIds.length,
      foldersProcessed: Object.keys(filesByFolder).length,
      totalFiles: Object.values(filesByFolder).reduce((total, files) => total + files.length, 0)
    };

    return NextResponse.json({
      success: true,
      summary,
      data: filesByFolder
    }, { status: 200 });

  } catch (error) {
    console.error('Failed to fetch files from Google Drive:', error);
    
    return NextResponse.json({ 
      error: 'Failed to fetch files from Google Drive',
      details: error instanceof Error ? error.message : 'Unknown error',
      timestamp: new Date().toISOString()
    }, { status: 500 });
  }
}


