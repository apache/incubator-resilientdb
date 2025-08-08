import { Footer } from 'nextra-theme-docs';
import { Anchor, Box } from '@mantine/core';

/**
 * You can customize the Nextra Footer component.
 * Don't forget to use the MantineProvider component.
 *
 * @since 1.0.0
 *
 */
export const MantineFooter = () => (
  <Box style={{ position: 'relative' }}>
    <Footer>
      © {new Date().getFullYear()} ResilientDB. Part of the  <Anchor href="https://resilientdb.incubator.apache.org/"> Apache Software Foundation</Anchor>.
    </Footer>
  </Box>
);