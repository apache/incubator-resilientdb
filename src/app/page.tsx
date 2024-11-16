import { Main, Section, Container, Box } from "@/components/craft";

export default function Home() {
  return (
    <Main>
    <Section>
      <Container>
        <h1>Heading</h1>
        <p>Content</p>
        <Box direction="row" wrap={true} gap={4}>
          <div>Item 1</div>
          <div>Item 2</div>
        </Box>
      </Container>
    </Section>
  </Main>
  );
}
