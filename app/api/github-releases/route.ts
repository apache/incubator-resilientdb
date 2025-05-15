export async function GET() {
  const response = await fetch(
    'https://api.github.com/repos/gfazioli/next-app-nextra-template/releases',
    {
      headers: {
        Accept: 'application/vnd.github+json',
        // Authorization: `Bearer ${process.env.GITHUB_TOKEN}`, // Optional for rate limit
      },
    }
  );
  if (!response.ok) {
    return Response.json({ error: 'Failed to fetch releases' }, { status: 500 });
  }
  const releases = await response.json();

  return Response.json({ releases, status: 'ok' });
}
