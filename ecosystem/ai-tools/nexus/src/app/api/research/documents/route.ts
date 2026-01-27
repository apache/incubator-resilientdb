/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

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


