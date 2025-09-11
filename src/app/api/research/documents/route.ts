// import { readdir, stat } from "fs/promises";
import { NextResponse } from "next/server";
// import { join } from "path";
import { getFilesFromFolder } from "@/lib/google-drive";

export async function GET() {
  // try {
  //   const documentsPath = join(process.cwd(), "documents");

  //   try {
  //     const files = await readdir(documentsPath);
  //     const documents = [];

  //     for (const file of files) {
  //       const filePath = join(documentsPath, file);
  //       const stats = await stat(filePath);

  //       // Only include PDF, DOC, DOCX, PPT, PPTX files
  //       const allowedExtensions = [".pdf", ".doc", ".docx", ".ppt", ".pptx"];
  //       const fileExtension = file
  //         .toLowerCase()
  //         .substring(file.lastIndexOf("."));

  //       if (stats.isFile() && allowedExtensions.includes(fileExtension)) {
  //         documents.push({
  //           id: file,
  //           name: file,
  //           path: `documents/${file}`,
  //           size: stats.size,
  //           uploadedAt: stats.mtime.toISOString(),
  //         });
  //       }
  //     }

  //     return NextResponse.json(documents);
  //   } catch (error) {
  //     console.error("Error reading documents directory:", error);
  //     return NextResponse.json([], { status: 200 }); // Return empty array if directory doesn't exist
  //   }
  // } catch (error) {
  //   console.error("Error in documents API:", error);
  //   return NextResponse.json(
  //     { error: "Failed to retrieve documents" },
  //     { status: 500 },
  //   );
  // }

  try {
    const folderId = "1ZnzPYX4E3SjcVpspJsDbj-1dK1sNfds8"; // Replace with your actual folder ID
    if (!folderId) {
      return NextResponse.json({ error: "Invalid Folder ID" }, { status: 400 });
    }
    
    console.log('Attempting to fetch files from Google Drive folder:', folderId);
    const files = await getFilesFromFolder(folderId);
    console.log('Successfully fetched files from Google Drive:', files);

    return NextResponse.json(files, { status: 200 });
  } catch (error) {
    console.error('Detailed API Error:', error);
    console.error('Error message:', error.message);
    console.error('Error stack:', error.stack);
    
    // Return more detailed error information
    return NextResponse.json({ 
      error: 'Failed to fetch files from Google Drive',
      details: error.message,
      folderId: "1ZnzPYX4E3SjcVpspJsDbj-1dK1sNfds8"
    }, { status: 500 });
  }

  return NextResponse.json([], { status: 200 });
}
