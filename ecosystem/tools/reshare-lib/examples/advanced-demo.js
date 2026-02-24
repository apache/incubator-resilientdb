import ResShareToolkitClient, { ApiError } from '../src/index.js';
import { mkdir, writeFile } from 'node:fs/promises';
import path from 'node:path';
import process from 'node:process';

const OUTPUT_DIR = 'ecosystem/tools/reshare-lib/examples/out';

function nowSlug() {
  const iso = new Date().toISOString();
  const compact = iso.replace(/[-:.TZ]/g, '').slice(0, 14);
  const random = Math.random().toString(16).slice(2, 8);
  return `${compact}-${random}`;
}

function getErrorStatus(error) {
  if (!(error instanceof ApiError)) return null;
  return error.body?.status || error.body?.result || error.body?.message || null;
}

async function safeSignup(client, { username, password }) {
  try {
    const result = await client.auth.signup({ username, password });
    console.log(`signup:${username}`, result.status);
  } catch (error) {
    const status = getErrorStatus(error);
    if (status === 'USER_EXISTS') {
      console.log(`signup:${username}`, status);
      return;
    }
    throw error;
  }
}

async function safeCreateFolder(client, folderPath) {
  try {
    const result = await client.files.createFolder(folderPath);
    console.log(`create-folder:${folderPath}`, result.status);
  } catch (error) {
    const status = getErrorStatus(error);
    if (status === 'DUPLICATE_NAME') {
      console.log(`create-folder:${folderPath}`, status);
      return;
    }
    throw error;
  }
}

async function safeShare(client, { target, path: nodePath }) {
  try {
    const result = await client.shares.share({ target, path: nodePath });
    console.log(`share:${nodePath}->${target}`, result.status);
  } catch (error) {
    const status = getErrorStatus(error);
    if (status === 'ALREADY_SHARED') {
      console.log(`share:${nodePath}->${target}`, status);
      return;
    }
    throw error;
  }
}

async function responseToFile(response, filePath) {
  const arrayBuffer = await response.arrayBuffer();
  const buffer = Buffer.from(arrayBuffer);
  await writeFile(filePath, buffer);
  return buffer.byteLength;
}

function findFirstFilePath(node, prefix = '') {
  if (!node || typeof node !== 'object') return null;
  const currentPath = prefix ? `${prefix}/${node.name}` : node.name;

  if (!node.is_folder) {
    return currentPath;
  }

  const children = node.children || {};
  const childNames = Object.keys(children).sort();
  for (const childName of childNames) {
    const result = findFirstFilePath(children[childName], currentPath);
    if (result) return result;
  }

  return null;
}

async function main() {
  const runId = nowSlug();
  const baseUrl = 'https://fc70-136-109-248-65.ngrok-free.app'; // Replace with ResilientDB ResShare backend URL
  const password = 'Pass@123';
  const owner = 'alice';
  const receiver = 'bob';

  await mkdir(OUTPUT_DIR, { recursive: true });

  const ownerClient = new ResShareToolkitClient({ baseUrl });
  const receiverClient = new ResShareToolkitClient({ baseUrl });

  await safeSignup(ownerClient, { username: owner, password });
  await safeSignup(receiverClient, { username: receiver, password });

  const ownerLogin = await ownerClient.auth.login({ username: owner, password });
  console.log(`login:${owner}`, ownerLogin.status);

  const rootFolder = `meeting-demo-${runId}`;
  const notesFolder = `${rootFolder}/notes`;
  const contractsFolder = `${rootFolder}/contracts`;

  await safeCreateFolder(ownerClient, rootFolder);
  await safeCreateFolder(ownerClient, notesFolder);
  await safeCreateFolder(ownerClient, contractsFolder);

  if (typeof Blob === 'undefined') {
    throw new Error('This example requires Blob support (Node.js >= 18).');
  }

  const meetingNotes = [
    'ResShare Meeting Notes',
    `Run ID: ${runId}`,
    'Project: Phoenix',
    'Launch date: 2026-03-15',
    'Owner: Devang',
    'Budget: $125k',
    '',
    'Action items:',
    '- Upload demo artifacts',
    '- Share folder with stakeholders',
    '- Validate shared downloads'
  ].join('\n');

  const contractText = [
    'Sample Contract Summary',
    `Run ID: ${runId}`,
    'Payment terms: Net 30',
    'Termination: 30 days notice',
    'Renewal: Annual'
  ].join('\n');

  const meetingNotesName = `meeting-notes-${runId}.txt`;
  const contractName = `contract-summary-${runId}.txt`;

  const uploadMeeting = await ownerClient.files.upload({
    file: new Blob([meetingNotes], { type: 'text/plain' }),
    filename: meetingNotesName,
    path: notesFolder,
    skipProcessing: false
  });
  console.log('upload', meetingNotesName, uploadMeeting.status, {
    rag_processed: uploadMeeting.rag_processed,
    rag_skipped: uploadMeeting.rag_skipped
  });

  const uploadContract = await ownerClient.files.upload({
    file: new Blob([contractText], { type: 'text/plain' }),
    filename: contractName,
    path: contractsFolder,
    skipProcessing: false
  });
  console.log('upload', contractName, uploadContract.status, {
    rag_processed: uploadContract.rag_processed,
    rag_skipped: uploadContract.rag_skipped
  });

  try {
    const stats = await ownerClient._requestJson('/chat/stats', { method: 'GET' });
    console.log('chat.stats', stats);
    const chat = await ownerClient._requestJson('/chat', {
      method: 'POST',
      body: { query: `What is the Phoenix launch date for run ${runId}?` },
      retries: 0
    });
    console.log('chat.answer', {
      chunks_found: chat.chunks_found,
      sources: chat.sources
    });
    console.log(chat.answer);
  } catch (error) {
    console.log('chat.error', error instanceof Error ? error.message : String(error));
  }

  const downloadPath = `${notesFolder}/${meetingNotesName}`;
  const fileResponse = await ownerClient.files.download({ path: downloadPath });
  const savedFile = path.join(OUTPUT_DIR, meetingNotesName);
  const fileBytes = await responseToFile(fileResponse, savedFile);
  console.log('download.saved', savedFile, `${fileBytes} bytes`);

  const zipResponse = await ownerClient.files.downloadZip({ path: rootFolder });
  const zipPath = path.join(OUTPUT_DIR, `${rootFolder}.zip`);
  const zipBytes = await responseToFile(zipResponse, zipPath);
  console.log('downloadZip.saved', zipPath, `${zipBytes} bytes`);

  await safeShare(ownerClient, { target: receiver, path: rootFolder });

  const receiverLogin = await receiverClient.auth.login({ username: receiver, password });
  console.log(`login:${receiver}`, receiverLogin.status);

  const shared = await receiverClient.shares.list();
  console.log('shared.list.keys', Object.keys(shared.share_list || {}));

  const fromUsers = Object.keys(shared.share_list || {}).sort();
  const firstFromUser = fromUsers[0];
  const firstNode = firstFromUser ? shared.share_list[firstFromUser]?.[0] : null;
  const firstFilePath = firstNode ? findFirstFilePath(firstNode) : null;
  if (!firstFromUser || !firstFilePath) {
    throw new Error('No shared file discovered; share list was empty or only contained folders with no files.');
  }

  const combinedFilePath = `${firstFromUser}/${firstFilePath}`;
  const sharedFileResponse = await receiverClient.files.download({ path: combinedFilePath, isShared: true });
  const receiverSavedPath = path.join(OUTPUT_DIR, `shared-from-${firstFromUser}-${path.basename(firstFilePath)}`);
  const sharedBytes = await responseToFile(sharedFileResponse, receiverSavedPath);
  console.log('shared.download.saved', receiverSavedPath, `${sharedBytes} bytes`);

  const combinedFolderPath = `${firstFromUser}/${firstNode.name}`;
  const sharedZipResponse = await receiverClient.files.downloadZip({ path: combinedFolderPath, isShared: true });
  const receiverZipPath = path.join(OUTPUT_DIR, `shared-from-${firstFromUser}-${firstNode.name}.zip`);
  const sharedZipBytes = await responseToFile(sharedZipResponse, receiverZipPath);
  console.log('shared.downloadZip.saved', receiverZipPath, `${sharedZipBytes} bytes`);

  const removeResult = await receiverClient.shares.remove({ combinedPath: `${firstFromUser}/${firstNode.name}` });
  console.log('shared.remove', removeResult.status);

  await receiverClient.auth.deleteUser({ password });
  console.log(`delete-user:${receiver}`, 'SUCCESS');
  await ownerClient.auth.deleteUser({ password });
  console.log(`delete-user:${owner}`, 'SUCCESS');
}

main().catch((error) => {
  if (error instanceof ApiError) {
    console.error('api.error', error.status, error.body);
    process.exitCode = 1;
    return;
  }
  console.error('example.failed', error instanceof Error ? error.stack || error.message : String(error));
  process.exitCode = 1;
});
