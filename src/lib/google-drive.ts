import { google } from 'googleapis';

// Initialize Google Drive API
export const getDriveService = (auth) => {
  return google.drive({ version: 'v3', auth });
};

// Get OAuth2 client
export const getGoogleAuth = () => {
  
  const auth = new google.auth.OAuth2(
    process.env.GOOGLE_CLIENT_ID,
    process.env.GOOGLE_CLIENT_SECRET,
    process.env.GOOGLE_REDIRECT_URI
  );

  const credentials = {
    access_token: process.env.GOOGLE_ACCESS_TOKEN,
    refresh_token: process.env.GOOGLE_REFRESH_TOKEN,
  };

  
  auth.setCredentials(credentials);
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
  }
};

// Get file content as base64 (for PDFs)
export const getFileContent = async (fileId:string, auth = null) => {
  try {
    const authClient = auth || getGoogleAuth();
    const drive = getDriveService(authClient);

    const response = await drive.files.get({
      fileId: fileId,
      alt: 'media'
    }, {
      responseType: 'arraybuffer'
    });

    // Convert to base64 for PDF viewing
    const base64 = Buffer.from(response?.data).toString('base64');
    return `data:application/pdf;base64,${base64}`;
  } catch (error) {
    console.error('Error getting file content:', error);
    throw error;
  }
};

// Get file metadata and content
export const getFileWithContent = async (fileId:string, auth = null) => {
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

    const base64Content = Buffer.from(contentResponse.data).toString('base64');

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