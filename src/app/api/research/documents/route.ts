// import { readdir, stat } from "fs/promises";
import { NextResponse } from "next/server";
// import { join } from "path";
import { getFilesFromFolder } from "@/lib/google-drive";

export async function GET() {

  try {
    const folderId = "1ZnzPYX4E3SjcVpspJsDbj-1dK1sNfds8"; // Replace with your actual folder ID
    if (!folderId) {
      return NextResponse.json({ error: "Invalid Folder ID" }, { status: 400 });
    }
    
    const files = await getFilesFromFolder(folderId);

    return NextResponse.json(files, { status: 200 });
  } catch (error) {
    return NextResponse.json({ 
      error: 'Failed to fetch files from Google Drive',
      details: error.message,
      folderId: "1ZnzPYX4E3SjcVpspJsDbj-1dK1sNfds8"
    }, { status: 500 });
  }

}
