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

import chalk from "chalk";
import { NextRequest, NextResponse } from "next/server";
import { config } from "../../../../config/environment";
import { configureLlamaSettings } from "../../../../lib/config/llama-settings";
import { llamaService } from "../../../../lib/llama-service";

export async function POST(req: NextRequest) {
  try {
    // Ensure LLM settings are configured
    configureLlamaSettings();

    const { documentPath, documentPaths } = await req.json();

    // Support both single document (backward compatibility) and multiple documents
    if (!documentPath && !documentPaths) {
      return NextResponse.json(
        {
          error: "Either documentPath or documentPaths is required",
        },
        { status: 400 },
      );
    }

    // Validate documentPaths if provided
    if (
      documentPaths &&
      (!Array.isArray(documentPaths) || documentPaths.length === 0)
    ) {
      return NextResponse.json(
        {
          error: "documentPaths must be a non-empty array",
        },
        { status: 400 },
      );
    }

    if (!config.deepSeekApiKey) {
      return NextResponse.json(
        { error: "DeepSeek API key is required" },
        { status: 500 },
      );
    }

    if (!config.llamaCloudApiKey) {
      return NextResponse.json(
        { error: "LlamaCloud API key is required" },
        { status: 500 },
      );
    }

    try {
      const pathsToProcess = documentPaths || [documentPath];

      console.log(
        chalk.blue(
          `[API] Prepare-Index: Starting ingestion for ${pathsToProcess.length} documents`,
        ),
      );
      const startTime = Date.now();

      await llamaService.ingestDocs(pathsToProcess);

      const totalTime = Date.now() - startTime;
      console.log(
        chalk.green(
          `[API] Prepare-Index: Ingestion completed in ${totalTime}ms`,
        ),
      );

      return NextResponse.json({
        success: true,
        message: `Documents prepared successfully`,
        documentCount: pathsToProcess.length,
        documentPaths: pathsToProcess,
        processingTimeMs: totalTime,
      });
    } catch (processingError) {
      console.error("Error preparing index:", processingError);

      // provide helpful error messages
      let errorMessage = "Failed to prepare document index";
      if (processingError instanceof Error) {
        if (
          processingError.message.includes("401") ||
          processingError.message.includes("unauthorized")
        ) {
          errorMessage =
            "Invalid API key. Please check your LlamaCloud or DeepSeek API keys.";
        } else if (
          processingError.message.includes("402") ||
          processingError.message.includes("payment")
        ) {
          errorMessage =
            "Insufficient credits. Please check your LlamaCloud account balance.";
        }
      }

      return NextResponse.json(
        {
          error: errorMessage,
          details:
            processingError instanceof Error
              ? processingError.message
              : String(processingError),
        },
        { status: 500 },
      );
    }
  } catch (error) {
    console.error("Error in prepare-index API:", error);
    return NextResponse.json(
      {
        error: "Internal server error",
        details: error instanceof Error ? error.message : String(error),
      },
      { status: 500 },
    );
  }
}
