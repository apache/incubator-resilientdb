import { readFile } from "fs/promises";
import { NextRequest, NextResponse } from "next/server";
import { join } from "path";

export async function GET(
  req: NextRequest,
  { params }: { params: Promise<{ path: string[] }> },
) {
  try {
    // Await params before accessing its properties
    const { path } = await params;

    // Reconstruct the file path
    const filePath = path.join("/");
    const fullPath = join(process.cwd(), filePath);

    // Security check - ensure we're only serving files from the documents folder
    if (!filePath.startsWith("documents/")) {
      return new NextResponse("Access denied", { status: 403 });
    }

    // Read the file
    const fileBuffer = await readFile(fullPath);

    // Determine content type based on file extension
    const extension = filePath.split(".").pop()?.toLowerCase();
    let contentType = "application/octet-stream";

    switch (extension) {
      case "pdf":
        contentType = "application/pdf";
        break;
      case "doc":
        contentType = "application/msword";
        break;
      case "docx":
        contentType =
          "application/vnd.openxmlformats-officedocument.wordprocessingml.document";
        break;
      case "ppt":
        contentType = "application/vnd.ms-powerpoint";
        break;
      case "pptx":
        contentType =
          "application/vnd.openxmlformats-officedocument.presentationml.presentation";
        break;
    }

    return new NextResponse(fileBuffer as any, {
      headers: {
        "Content-Type": contentType,
        "Content-Disposition": "inline", // Display in browser instead of downloading
      },
    });
  } catch (error) {
    console.error("Error serving file:", error);
    return new NextResponse("File not found", { status: 404 });
  }
}
