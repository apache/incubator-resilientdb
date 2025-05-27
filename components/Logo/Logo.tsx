import Image from 'next/image';

export function Logo() {
  return <Image src="/lighthouse.png" alt="Logo" width={48} height={48} style={{ marginRight: '12px' }} />;
}
