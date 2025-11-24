import { Container } from '@mantine/core';
import ShaderBackground from '@/components/landing/ShaderBackground';
import PulsingCircle from '@/components/landing/PulsingCircle';
import HeroContent from '@/components/landing/HeroContent';
import Header from '@/components/landing/Header';

export default function HomePage() {
  return (
    <Container p={0} size="100%" style={{ maxWidth: '100%' }}>
      <ShaderBackground>
        <Header />
        <HeroContent />
        <PulsingCircle />
      </ShaderBackground>
    </Container>
  );
}
