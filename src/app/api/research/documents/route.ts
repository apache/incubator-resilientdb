import { readdir, stat } from "fs/promises";
import { NextResponse } from "next/server";
import { join } from "path";

export async function GET() {
    try {
        const documentsPath = join(process.cwd(), "documents");

        try {
            const files = await readdir(documentsPath);
            const documents = [];

            for (const file of files) {
                const filePath = join(documentsPath, file);
                const stats = await stat(filePath);

                // Only include PDF, DOC, DOCX, PPT, PPTX files
                const allowedExtensions = ['.pdf', '.doc', '.docx', '.ppt', '.pptx'];
                const fileExtension = file.toLowerCase().substring(file.lastIndexOf('.'));

                if (stats.isFile() && allowedExtensions.includes(fileExtension)) {
                    documents.push({
                        id: file,
                        name: file,
                        path: `documents/${file}`,
                        size: stats.size,
                        uploadedAt: stats.mtime.toISOString(),
                    });
                }
            }

            return NextResponse.json(documents);
        } catch (error) {
            console.error("Error reading documents directory:", error);
            return NextResponse.json([], { status: 200 }); // Return empty array if directory doesn't exist
        }
    } catch (error) {
        console.error("Error in documents API:", error);
        return NextResponse.json(
            { error: "Failed to retrieve documents" },
            { status: 500 }
        );
    }
} 