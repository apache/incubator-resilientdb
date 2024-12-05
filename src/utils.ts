export function parseResilientURL(url: string): string {
  const regex = /^resilient:\/\/(.+)$/;
  const match = url.match(regex);

  if (!match) {
    throw new Error('Invalid Resilient URL format. Expected resilient://<host>');
  }

  return `https://${match[1]}`;
}
