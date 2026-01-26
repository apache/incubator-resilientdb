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

import { google } from 'googleapis';

// Initialize Google Drive API
export const getDriveService = (auth: any) => {
  return google.drive({ version: 'v3', auth });
};

// Get OAuth2 client with automatic token refresh
export const getGoogleAuth = () => {
  
  const auth = new google.auth.OAuth2(
    process.env.GOOGLE_CLIENT_ID,
    process.env.GOOGLE_CLIENT_SECRET,
    process.env.GOOGLE_REDIRECT_URI
  );

  // Set credentials
  auth.setCredentials({
    refresh_token: process.env.GOOGLE_REFRESH_TOKEN,
  });

  // Auto-refresh tokens and optionally save new ones
  auth.on('tokens', (tokens) => {
    // If you get a new refresh token, you should save it
    if (tokens.refresh_token) {
      console.log('⚠️  New refresh token received. Consider updating your env file.');
      // In production, you'd save this to your database or secret manager
    }
    
    // Access token is refreshed automatically - no action needed
    if (tokens.access_token) {
      console.log('✅ New access token obtained');
    }
  });

  return auth;
};

// Get files from a specific folder
export const getFilesFromFolder = async (folderId: string, auth = null) => {
  try {
    const authClient = auth || getGoogleAuth();
    
    const drive = getDriveService(authClient);
    const response = await drive.files.list({
      q: `'${folderId}' in parents and trashed = false and mimeType='application/pdf'`,
      fields: 'files(id, name, mimeType, size, createdTime, modifiedTime, webViewLink, webContentLink, thumbnailLink)',
      orderBy: 'name'
    });
    console.log(response.data.files);
    return response.data.files;

  } catch (error) {
    console.error('Error fetching files from folder:', error);
    
    if (
      typeof error === 'object' &&
      error !== null &&
      ('code' in error || 'message' in error)
    ) {
      const err = error as { code?: number; message?: string };
      if (err.code === 401 || err.message?.includes('invalid_grant')) {
        throw new Error('Google authentication failed. Please regenerate your access and refresh tokens.');
      }
    }
    
    throw error;
  }
};

// Get file content as base64 (for PDFs)
export const getFileContent = async (fileId:string, auth = null) => {
  try {
    const authClient = auth || getGoogleAuth();
    const drive = getDriveService(authClient);

    const response = await drive.files.get(
      {
        fileId: fileId,
        alt: 'media'
      },
      {
        responseType: 'arraybuffer'
      }
    );

    // Convert to base64 for PDF viewing
    const base64 = Buffer.from(response.data as ArrayBuffer).toString('base64');
    return `data:application/pdf;base64,${base64}`;
  } catch (error) {
    console.error('Error getting file content:', error);
    throw error;
  }
};

// Get file metadata and content
export const getFileWithContent = async (fileId: string, auth = null) => {
  try {
    const authClient = auth || getGoogleAuth();
    const drive = getDriveService(authClient);

    // Get file metadata
    const metadataResponse = await drive.files.get({
      fileId: fileId,
      fields: 'id, name, mimeType, size, createdTime, modifiedTime, webViewLink'
    });

    // Get file content
    const contentResponse = await drive.files.get({
      fileId: fileId,
      alt: 'media'
    }, {
      responseType: 'arraybuffer'
    });

    const base64Content = Buffer.from(contentResponse.data as ArrayBuffer).toString('base64');

    return {
      metadata: metadataResponse.data,
      content: `data:application/pdf;base64,${base64Content}`,
      downloadUrl: `data:application/pdf;base64,${base64Content}`
    };
  } catch (error) {
    console.error('Error getting file with content:', error);
    throw error;
  }
};

export async function getFilesFromMultipleFolders(folderIds: string[], auth = null): Promise<Record<string, any[]>> {
  try {
    const authClient = auth || getGoogleAuth();
    const drive = getDriveService(authClient);
    const results: Record<string, any[]> = {};

    await Promise.all(
      folderIds.map(async (folderId) => {
        try {
          const query = `'${folderId}' in parents and trashed=false`;
          const res = await drive.files.list({
            q: query,
            fields: 'files(id,name,mimeType,createdTime,modifiedTime,size,webViewLink)',
            pageSize: 1000,
          });
          results[folderId] = res.data.files || [];
        } catch (err) {
          console.error(`Error for folder ${folderId}:`, err);
          results[folderId] = [];
        }
      })
    );


    return results;
   
  } catch (error) {
    console.error('Error fetching files from multiple folders:', error);
    throw error;
  }
}