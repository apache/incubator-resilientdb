"use client"

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

import { Anchor, Box, Container, SimpleGrid, Stack, Group, Text, Divider, ActionIcon, Title } from "@mantine/core"
import { IconBrandGithub, IconWorldWww, IconMail } from "@tabler/icons-react"

/**
 * You can customize the Nextra Footer component.
 * Don't forget to use the MantineProvider component.
 *
 * @since 1.0.0
 *
 */
export const MantineFooter = () => (
  <Box 
    component="footer" 
    style={{ 
      position: "relative", 
      marginBottom: "0",
      background: "linear-gradient(135deg, rgba(81, 207, 102, 0.05) 0%, rgba(59, 130, 246, 0.08) 50%, rgba(139, 92, 246, 0.05) 100%)",
      borderTop: "1px solid rgba(81, 207, 102, 0.2)",
      backdropFilter: "blur(10px)",
      overflow: "hidden",
      paddingTop: "2rem",
      paddingBottom: "0.5rem",
    }}
  >
      {/* Animated background elements */}
      <Box
        style={{
          position: "absolute",
          top: 0,
          left: 0,
          right: 0,
          bottom: 0,
          background: "radial-gradient(circle at 20% 80%, rgba(81, 207, 102, 0.1) 0%, transparent 50%), radial-gradient(circle at 80% 20%, rgba(59, 130, 246, 0.1) 0%, transparent 50%)",
          opacity: 0.6,
          pointerEvents: "none",
        }}
      />
      <Container size="lg">
        <SimpleGrid cols={{ base: 1, sm: 2, md: 3 }} spacing="xl">
          <Stack gap="xs" style={{ alignItems: "flex-start" }}>
            <Title
              order={4}
              style={{
                transition: "all 0.3s ease",
                cursor: "default",
                color: "#51cf66",
                fontWeight: 700,
                textShadow: "0 1px 2px rgba(81, 207, 102, 0.3)",
                fontSize: "1.5rem",
                textAlign: "left",
                marginBottom: "0.5rem",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.transform = "translateY(-2px)"
                e.currentTarget.style.color = "#40c057"
                e.currentTarget.style.textShadow = "0 2px 4px rgba(81, 207, 102, 0.5)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.transform = "translateY(0)"
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.textShadow = "0 1px 2px rgba(81, 207, 102, 0.3)"
              }}
            >
              Docs
            </Title>
            <Anchor
              href="/docs"
              style={{
                color: "rgba(255, 255, 255, 0.8)",
                textDecoration: "none",
                transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                padding: "6px 12px",
                borderRadius: "8px",
                position: "relative",
                overflow: "hidden",
                fontWeight: 500,
                border: "1px solid transparent",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.15)"
                e.currentTarget.style.transform = "translateX(6px) translateY(-1px)"
                e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.3)"
                e.currentTarget.style.boxShadow = "0 4px 12px rgba(81, 207, 102, 0.2)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                e.currentTarget.style.backgroundColor = "transparent"
                e.currentTarget.style.transform = "translateX(0) translateY(0)"
                e.currentTarget.style.borderColor = "transparent"
                e.currentTarget.style.boxShadow = "none"
              }}
            >
              Overview
            </Anchor>
            <Anchor
              href="/docs/installation"
              style={{
                color: "rgba(255, 255, 255, 0.8)",
                textDecoration: "none",
                transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                padding: "6px 12px",
                borderRadius: "8px",
                position: "relative",
                overflow: "hidden",
                fontWeight: 500,
                border: "1px solid transparent",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.15)"
                e.currentTarget.style.transform = "translateX(6px) translateY(-1px)"
                e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.3)"
                e.currentTarget.style.boxShadow = "0 4px 12px rgba(81, 207, 102, 0.2)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                e.currentTarget.style.backgroundColor = "transparent"
                e.currentTarget.style.transform = "translateX(0) translateY(0)"
                e.currentTarget.style.borderColor = "transparent"
                e.currentTarget.style.boxShadow = "none"
              }}
            >
              Installation
            </Anchor>
            <Anchor
              href="/docs/resilientdb"
              style={{
                color: "rgba(255, 255, 255, 0.8)",
                textDecoration: "none",
                transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                padding: "6px 12px",
                borderRadius: "8px",
                position: "relative",
                overflow: "hidden",
                fontWeight: 500,
                border: "1px solid transparent",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.15)"
                e.currentTarget.style.transform = "translateX(6px) translateY(-1px)"
                e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.3)"
                e.currentTarget.style.boxShadow = "0 4px 12px rgba(81, 207, 102, 0.2)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                e.currentTarget.style.backgroundColor = "transparent"
                e.currentTarget.style.transform = "translateX(0) translateY(0)"
                e.currentTarget.style.borderColor = "transparent"
                e.currentTarget.style.boxShadow = "none"
              }}
            >
              ResilientDB
            </Anchor>
          </Stack>

          <Stack gap="xs" style={{ alignItems: "flex-start" }}>
            <Title
              order={4}
              style={{
                transition: "all 0.3s ease",
                cursor: "default",
                color: "#51cf66",
                fontWeight: 700,
                textShadow: "0 1px 2px rgba(81, 207, 102, 0.3)",
                fontSize: "1.5rem",
                textAlign: "left",
                marginBottom: "0.5rem",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.transform = "translateY(-2px)"
                e.currentTarget.style.color = "#40c057"
                e.currentTarget.style.textShadow = "0 2px 4px rgba(81, 207, 102, 0.5)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.transform = "translateY(0)"
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.textShadow = "0 1px 2px rgba(81, 207, 102, 0.3)"
              }}
            >
              Ecosystem
            </Title>
            <Anchor
              href="/docs/resilientdb_graphql"
              style={{
                color: "rgba(255, 255, 255, 0.8)",
                textDecoration: "none",
                transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                padding: "6px 12px",
                borderRadius: "8px",
                position: "relative",
                overflow: "hidden",
                fontWeight: 500,
                border: "1px solid transparent",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.15)"
                e.currentTarget.style.transform = "translateX(6px) translateY(-1px)"
                e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.3)"
                e.currentTarget.style.boxShadow = "0 4px 12px rgba(81, 207, 102, 0.2)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                e.currentTarget.style.backgroundColor = "transparent"
                e.currentTarget.style.transform = "translateX(0) translateY(0)"
                e.currentTarget.style.borderColor = "transparent"
                e.currentTarget.style.boxShadow = "none"
              }}
            >
              ResilientDB GraphQL
            </Anchor>
            <Anchor
              href="/docs/reslens"
              style={{
                color: "rgba(255, 255, 255, 0.8)",
                textDecoration: "none",
                transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                padding: "6px 12px",
                borderRadius: "8px",
                position: "relative",
                overflow: "hidden",
                fontWeight: 500,
                border: "1px solid transparent",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.15)"
                e.currentTarget.style.transform = "translateX(6px) translateY(-1px)"
                e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.3)"
                e.currentTarget.style.boxShadow = "0 4px 12px rgba(81, 207, 102, 0.2)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                e.currentTarget.style.backgroundColor = "transparent"
                e.currentTarget.style.transform = "translateX(0) translateY(0)"
                e.currentTarget.style.borderColor = "transparent"
                e.currentTarget.style.boxShadow = "none"
              }}
            >
              ResLens – Monitoring
            </Anchor>
            <Anchor
              href="/docs/resvault"
              style={{
                color: "rgba(255, 255, 255, 0.8)",
                textDecoration: "none",
                transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                padding: "6px 12px",
                borderRadius: "8px",
                position: "relative",
                overflow: "hidden",
                fontWeight: 500,
                border: "1px solid transparent",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.15)"
                e.currentTarget.style.transform = "translateX(6px) translateY(-1px)"
                e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.3)"
                e.currentTarget.style.boxShadow = "0 4px 12px rgba(81, 207, 102, 0.2)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                e.currentTarget.style.backgroundColor = "transparent"
                e.currentTarget.style.transform = "translateX(0) translateY(0)"
                e.currentTarget.style.borderColor = "transparent"
                e.currentTarget.style.boxShadow = "none"
              }}
            >
              ResVault
            </Anchor>
          </Stack>

          <Stack gap="xs" style={{ alignItems: "flex-start" }}>
            <Title
              order={4}
              style={{
                transition: "all 0.3s ease",
                cursor: "default",
                color: "#51cf66",
                fontWeight: 700,
                textShadow: "0 1px 2px rgba(81, 207, 102, 0.3)",
                fontSize: "1.5rem",
                textAlign: "left",
                marginBottom: "0.5rem",
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.transform = "translateY(-2px)"
                e.currentTarget.style.color = "#40c057"
                e.currentTarget.style.textShadow = "0 2px 4px rgba(81, 207, 102, 0.5)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.transform = "translateY(0)"
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.textShadow = "0 1px 2px rgba(81, 207, 102, 0.3)"
              }}
            >
              Community
            </Title>
            <Group gap="xs">
              <ActionIcon
                component="a"
                href="https://github.com/apache"
                target="_blank"
                aria-label="GitHub"
                variant="subtle"
                style={{
                  transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                  borderRadius: "12px",
                  color: "rgba(255, 255, 255, 0.8)",
                  border: "1px solid rgba(81, 207, 102, 0.2)",
                  background: "rgba(81, 207, 102, 0.05)",
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.2)"
                  e.currentTarget.style.color = "#51cf66"
                  e.currentTarget.style.transform = "translateY(-3px) scale(1.1)"
                  e.currentTarget.style.boxShadow = "0 8px 20px rgba(81, 207, 102, 0.3)"
                  e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.5)"
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.05)"
                  e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                  e.currentTarget.style.transform = "translateY(0) scale(1)"
                  e.currentTarget.style.boxShadow = "none"
                  e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.2)"
                }}
              >
                <IconBrandGithub size={20} />
              </ActionIcon>
              <ActionIcon
                component="a"
                href="https://resilientdb.com"
                target="_blank"
                aria-label="Website"
                variant="subtle"
                style={{
                  transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                  borderRadius: "12px",
                  color: "rgba(255, 255, 255, 0.8)",
                  border: "1px solid rgba(81, 207, 102, 0.2)",
                  background: "rgba(81, 207, 102, 0.05)",
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.2)"
                  e.currentTarget.style.color = "#51cf66"
                  e.currentTarget.style.transform = "translateY(-3px) scale(1.1)"
                  e.currentTarget.style.boxShadow = "0 8px 20px rgba(81, 207, 102, 0.3)"
                  e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.5)"
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.05)"
                  e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                  e.currentTarget.style.transform = "translateY(0) scale(1)"
                  e.currentTarget.style.boxShadow = "none"
                  e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.2)"
                }}
              >
                <IconWorldWww size={20} />
              </ActionIcon>
              <ActionIcon
                component="a"
                href="mailto:contact@resilientdb.com"
                aria-label="Email"
                variant="subtle"
                style={{
                  transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                  borderRadius: "12px",
                  color: "rgba(255, 255, 255, 0.8)",
                  border: "1px solid rgba(81, 207, 102, 0.2)",
                  background: "rgba(81, 207, 102, 0.05)",
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.2)"
                  e.currentTarget.style.color = "#51cf66"
                  e.currentTarget.style.transform = "translateY(-3px) scale(1.1)"
                  e.currentTarget.style.boxShadow = "0 8px 20px rgba(81, 207, 102, 0.3)"
                  e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.5)"
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.backgroundColor = "rgba(81, 207, 102, 0.05)"
                  e.currentTarget.style.color = "rgba(255, 255, 255, 0.8)"
                  e.currentTarget.style.transform = "translateY(0) scale(1)"
                  e.currentTarget.style.boxShadow = "none"
                  e.currentTarget.style.borderColor = "rgba(81, 207, 102, 0.2)"
                }}
              >
                <IconMail size={20} />
              </ActionIcon>
            </Group>
            <Text size="sm" c="dimmed" style={{ color: "rgba(255, 255, 255, 0.7)", fontWeight: 500 }}>
              Docs and tools to help you build with ResilientDB.
            </Text>
          </Stack>
        </SimpleGrid>
        
        {/* Integrated copyright section */}
        <Box
          mt="lg"
          pt="md"
          style={{
            borderTop: "1px solid rgba(81, 207, 102, 0.2)",
            background: "linear-gradient(90deg, transparent 0%, rgba(81, 207, 102, 0.05) 50%, transparent 100%)",
            borderRadius: "8px",
            padding: "8px 12px",
            marginTop: "1rem",
            marginBottom: "0",
          }}
        >
          <Text
            size="sm"
            ta="center"
            style={{
              color: "rgba(255, 255, 255, 0.8)",
              transition: "color 0.3s ease",
              fontWeight: 500,
              display: "flex",
              alignItems: "center",
              justifyContent: "center",
              gap: "8px",
            }}
          >
            <span>© {new Date().getFullYear()} ResilientDB. Part of the</span>
            <Anchor
              href="https://resilientdb.incubator.apache.org/"
              style={{
                color: "#51cf66",
                textDecoration: "none",
                transition: "all 0.3s cubic-bezier(0.4, 0, 0.2, 1)",
                position: "relative",
                fontWeight: 600,
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.color = "#40c057"
                e.currentTarget.style.textShadow = "0 1px 2px rgba(81, 207, 102, 0.3)"
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.color = "#51cf66"
                e.currentTarget.style.textShadow = "none"
              }}
            >
              Apache Software Foundation
            </Anchor>
            <span>.</span>
        </Text>
        </Box>
      </Container>
  </Box>
)
