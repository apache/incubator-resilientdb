import pack from '../../../package.json';

export async function GET() {
  return Response.json({ version: pack.version, status: 'ok' });
}
