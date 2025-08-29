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

'use client';

import { useState, useEffect, useRef } from 'react';
import { 
  Container, 
  Title, 
  Text, 
  Button, 
  Card, 
  Badge, 
  Group, 
  Stack, 
  Box,
  ActionIcon,
  Flex
} from '@mantine/core';
import { 
  IconExternalLink, 
  IconBrandGithub, 
  IconTerminal, 
  IconBolt, 
  IconDatabase, 
  IconCode, 
  IconWorld, 
  IconChevronLeft, 
  IconChevronRight 
} from '@tabler/icons-react';
import Link from "next/link";
import classes from './page.module.css';

// Import actual package.json data
import pack from '../package.json';

// Ecosystem projects data
const ecosystemProjects = [
  {
    name: "ResilientDB",
    href: "https://github.com/apache/incubator-resilientdb",
    description: "Core blockchain infrastructure for high-performance distributed systems",
    icon: IconDatabase
  },
  {
    name: "ResilientDB GraphQL",
    href: "https://github.com/apache/incubator-resilientdb-graphql",
    description: "GraphQL interface for seamless blockchain data querying",
    icon: IconWorld
  },
  {
    name: "Resilient-Python-Cache",
    href: "https://github.com/apache/incubator-resilientdb-resilient-python-cache",
    description: "High-performance Python caching solution for blockchain applications",
    icon: IconBolt
  },
  {
    name: "ResLens",
    href: "https://github.com/harish876/ResLens",
    description: "Advanced analytics and monitoring tool for ResilientDB networks",
    icon: IconTerminal
  },
  {
    name: "ResilientDB SDK",
    href: "https://github.com/apache/incubator-resilientdb",
    description: "Comprehensive software development kit for blockchain integration",
    icon: IconCode
  },
  {
    name: "ResilientDB CLI",
    href: "https://github.com/apache/incubator-resilientdb",
    description: "Command line interface for blockchain network management",
    icon: IconTerminal
  }
];

export default function ResilientDBLanding() {
  const [typewriterText, setTypewriterText] = useState('');
  const [currentLine, setCurrentLine] = useState(0);
  const [currentIndex, setCurrentIndex] = useState(Math.floor((ecosystemProjects.length - 3) / 2));
  const terminalRef = useRef<HTMLPreElement>(null);
  
  const terminalLines = [
    '🔗 ResilientDB Node Dependencies:',
    ...Object.keys(pack.dependencies).map(
      (key) => `• ${key} : ${pack.dependencies[key as keyof typeof pack.dependencies]}`
    ),
    '',
    'Each dependency is a block in your development chain.',
    'Keep your stack resilient!'
  ];

  useEffect(() => {
    if (currentLine < terminalLines.length) {
      const timer = setTimeout(() => {
        setTypewriterText(prev => prev + terminalLines[currentLine] + '\n');
        setCurrentLine(prev => prev + 1);
      }, 200);
      return () => clearTimeout(timer);
    }
  }, [currentLine, terminalLines]);

  // Auto-scroll to bottom when text updates
  useEffect(() => {
    if (terminalRef.current) {
      terminalRef.current.scrollTop = terminalRef.current.scrollHeight;
    }
  }, [typewriterText]);

  const cardWidth = 320;
  const canGoNext = currentIndex < ecosystemProjects.length - 3;
  const canGoPrev = currentIndex > 0;

  const nextSlide = () => {
    if (canGoNext) {
      setCurrentIndex((prev) => prev + 1);
    }
  };

  const prevSlide = () => {
    if (canGoPrev) {
      setCurrentIndex((prev) => prev - 1);
    }
  };

  const getCardStyle = (index: number) => {
    const relativePos = index - currentIndex;
    
    let scale = 1;
    let opacity = 1;
    let blur = 0;
    let zIndex = 5;

    if (relativePos >= 0 && relativePos <= 2) {
      scale = 1;
      opacity = 1;
      blur = 0;
      zIndex = 10;
    }
    else if (relativePos === -1) {
      if (canGoPrev) {
        scale = 0.75;
        opacity = 0.5;
        blur = 2;
        zIndex = 5;
      } else {
        scale = 0;
        opacity = 0;
        blur = 0;
        zIndex = 1;
      }
    }
    else if (relativePos === 3) {
      if (canGoNext) {
        scale = 0.75;
        opacity = 0.5;
        blur = 2;
        zIndex = 5;
      } else {
        scale = 0;
        opacity = 0;
        blur = 0;
        zIndex = 1;
      }
    }
    else {
      scale = 0;
      opacity = 0;
      blur = 0;
      zIndex = 1;
    }

    return {
      transform: `scale(${scale})`,
      opacity,
      filter: `blur(${blur}px)`,
      zIndex
    };
  };

  return (
        <Box
      style={{
        minHeight: '100vh',
        width: '100%'
      }}
      className={classes.root}
    >
      {/* Hero Section */}
      <Box 
        style={{
          position: 'relative',
          overflow: 'hidden',
          padding: '5rem 0'
        }}
        className={classes.hero}
      >
        <Box 
          style={{
            position: 'absolute',
            inset: 0,
            backgroundImage: 'radial-gradient(circle, rgba(255, 255, 255, 0.02) 1px, transparent 1px)',
            backgroundSize: '50px 50px'
          }}
        />
        <Container size="lg" style={{ position: 'relative', zIndex: 1 }}>
          <Stack align="center" gap="xl">
            <Stack align="center" gap="md">
              <Stack align="center" gap="md">
                <Title 
                  order={1}
                  ta="center"
                  className={classes.heroTitle}
                  style={{
                    fontSize: 'clamp(2.5rem, 8vw, 4.5rem)',
                    fontWeight: 800,
                    lineHeight: 1.1
                  }}
                >
                  Next Gen Docs for
                </Title>
                <Title 
                  order={1}
                  ta="center"
                  style={{
                    fontSize: 'clamp(2.5rem, 8vw, 4.5rem)',
                    fontWeight: 800,
                    lineHeight: 1.1
                  }}
                >
                  <Text 
                    component="span"
                    className={classes.animatedGradient}
                    style={{
                      fontSize: 'inherit',
                      fontWeight: 'inherit'
                    }}
                  >
                    ResilientDB
                  </Text>
                </Title>
              </Stack>
              
              <Group justify="center" gap="sm">
                <Badge 
                  variant="outline" 
                  color="teal"
                  leftSection={<IconBolt size={12} />}
                >
                  High Performance
                </Badge>
                <Badge 
                  variant="outline" 
                  color="blue"
                  leftSection={<IconDatabase size={12} />}
                >
                  Blockchain
                </Badge>
                <Badge 
                  variant="outline" 
                  color="gray"
                  leftSection={<IconCode size={12} />}
                >
                  Developer Friendly
                </Badge>
              </Group>
            </Stack>

            <Text 
              size="lg" 
              ta="center" 
              maw={800}
              lh={1.6}
              className={classes.description}
            >
              Find all documentation related to ResilientDB, its applications, and ecosystem tools
              supported by the ResilientDB team. This site is your gateway to high-performance blockchain
              infrastructure, developer guides, and integration resources. To learn more about
              ResilientDB, visit{' '}
              <Text 
                component={Link} 
                href="https://resilientdb.com/" 
                style={{ 
                  color: '#51cf66',
                  textDecoration: 'none',
                  padding: '2px 6px',
                  borderRadius: '4px',
                  background: 'rgba(81, 207, 102, 0.1)',
                  border: '1px solid rgba(81, 207, 102, 0.2)',
                  transition: 'all 0.3s ease',
                  cursor: 'pointer',
                  fontWeight: 500
                }}
                onMouseEnter={(e) => {
                  e.currentTarget.style.background = 'rgba(81, 207, 102, 0.2)';
                  e.currentTarget.style.borderColor = 'rgba(81, 207, 102, 0.4)';
                  e.currentTarget.style.transform = 'translateY(-1px)';
                }}
                onMouseLeave={(e) => {
                  e.currentTarget.style.background = 'rgba(81, 207, 102, 0.1)';
                  e.currentTarget.style.borderColor = 'rgba(81, 207, 102, 0.2)';
                  e.currentTarget.style.transform = 'translateY(0)';
                }}
              >
                the official website
              </Text>.
            </Text>

            <Group justify="center" gap="md">
              <Button 
                component={Link}
                href="https://resilientdb.incubator.apache.org/"
                size="lg"
                radius="xl"
                variant="gradient"
                gradient={{ from: 'gray.7', to: 'gray.6' }}
                leftSection={<IconBrandGithub size={20} />}
                rightSection={<IconExternalLink size={16} />}
                style={{
                  transition: 'all 0.3s ease',
                  border: '1px solid #868e96'
                }}
              >
                Latest Release v{pack.version}
              </Button>
              
              <Button 
                size="lg"
                radius="xl"
                variant="outline"
                color="gray"
                leftSection={<IconWorld size={20} />}
                style={{
                  background: 'rgba(37, 38, 43, 0.5)',
                  backdropFilter: 'blur(10px)',
                  transition: 'all 0.3s ease'
                }}
              >
                Documentation
              </Button>
            </Group>
          </Stack>
        </Container>
      </Box>

      {/* Terminal Section */}
      <Container size="lg" py="xl">
        <Card 
          radius="md"
          shadow="xl"
          style={{
            background: '#1a1b23',
            border: '1px solid #373a40',
            maxWidth: '800px',
            margin: '0 auto'
          }}
        >
          <Group justify="space-between" mb="md">
            <Group gap="xs">
              <IconTerminal size={20} color="#51cf66" />
              <Text size="sm" c="dimmed">Dependencies</Text>
            </Group>
            <Group gap={4}>
              <Box 
                style={{
                  width: '12px',
                  height: '12px',
                  borderRadius: '50%',
                  background: '#fa5252'
                }}
              />
              <Box 
                style={{
                  width: '12px',
                  height: '12px',
                  borderRadius: '50%',
                  background: '#fd7e14'
                }}
              />
              <Box 
                style={{
                  width: '12px',
                  height: '12px',
                  borderRadius: '50%',
                  background: '#51cf66'
                }}
              />
            </Group>
          </Group>
          
          <Box 
            ref={terminalRef}
            component="pre"
            style={{
              color: '#51cf66',
              fontFamily: "'Monaco', 'Menlo', 'Ubuntu Mono', monospace",
              fontSize: '0.875rem',
              lineHeight: 1.6,
              whiteSpace: 'pre-wrap',
              height: '250px',
              maxHeight: '250px',
              overflowY: 'auto',
              margin: 0,
              padding: '1rem',
              scrollbarWidth: 'thin',
              scrollbarColor: '#373a40 #1a1b23'
            }}
          >
            {typewriterText}
          </Box>
        </Card>
      </Container>

      {/* Ecosystem Section */}
      <Container size="xl" py="xl">
        <Stack align="center" gap="xl">
          <Stack align="center" gap="md">
            <Title order={2} ta="center" className={classes.heroTitle}>
              Explore Apps and Ecosystem
            </Title>
            <Text c="dimmed" size="lg" ta="center">
              Discover the tools and applications built on ResilientDB
            </Text>
            <Text 
              component={Link}
              href="https://github.com/ResilientApp"
              style={{ 
                color: '#51cf66',
                textDecoration: 'none',
                padding: '6px 12px',
                borderRadius: '6px',
                background: 'rgba(81, 207, 102, 0.1)',
                border: '1px solid rgba(81, 207, 102, 0.2)',
                transition: 'all 0.3s ease',
                cursor: 'pointer',
                fontWeight: 500,
                display: 'inline-flex',
                alignItems: 'center',
                gap: '6px'
              }}
              onMouseEnter={(e) => {
                e.currentTarget.style.background = 'rgba(81, 207, 102, 0.2)';
                e.currentTarget.style.borderColor = 'rgba(81, 207, 102, 0.4)';
                e.currentTarget.style.transform = 'translateY(-1px)';
              }}
              onMouseLeave={(e) => {
                e.currentTarget.style.background = 'rgba(81, 207, 102, 0.1)';
                e.currentTarget.style.borderColor = 'rgba(81, 207, 102, 0.2)';
                e.currentTarget.style.transform = 'translateY(0)';
              }}
            >
              Visit the ResilientDB Hub for more projects
              <IconExternalLink size={16} />
            </Text>
          </Stack>

          {/* Carousel */}
          <Box 
            style={{
              position: 'relative',
              width: '100%',
              maxWidth: '1400px'
            }}
          >
            {/* Left Arrow */}
            <ActionIcon
              onClick={prevSlide}
              disabled={!canGoPrev}
              variant="outline"
              size="lg"
              radius="md"
              style={{
                position: 'absolute',
                left: '1rem',
                top: '50%',
                transform: 'translateY(-50%)',
                zIndex: 20,
                backdropFilter: 'blur(10px)',
                transition: 'all 0.3s ease',
                background: canGoPrev ? 'rgba(37, 38, 43, 0.9)' : 'rgba(26, 27, 35, 0.5)',
                borderColor: canGoPrev ? '#868e96' : 'rgba(55, 58, 64, 0.5)',
                color: canGoPrev ? 'var(--mantine-color-text)' : '#868e96',
                cursor: canGoPrev ? 'pointer' : 'not-allowed'
              }}
            >
              <IconChevronLeft size={24} />
            </ActionIcon>

            {/* Right Arrow */}
            <ActionIcon
              onClick={nextSlide}
              disabled={!canGoNext}
              variant="outline"
              size="lg"
              radius="md"
              style={{
                position: 'absolute',
                right: '1rem',
                top: '50%',
                transform: 'translateY(-50%)',
                zIndex: 20,
                backdropFilter: 'blur(10px)',
                transition: 'all 0.3s ease',
                background: canGoNext ? 'rgba(37, 38, 43, 0.9)' : 'rgba(26, 27, 35, 0.5)',
                borderColor: canGoNext ? '#868e96' : 'rgba(55, 58, 64, 0.5)',
                color: canGoNext ? 'var(--mantine-color-text)' : '#868e96',
                cursor: canGoNext ? 'pointer' : 'not-allowed'
              }}
            >
              <IconChevronRight size={24} />
            </ActionIcon>

            {/* Cards Container */}
            <Box 
              style={{
                overflow: 'hidden',
                padding: '0 150px'
              }}
            >
              <Flex 
                style={{
                  transition: 'transform 0.7s cubic-bezier(0.4, 0, 0.2, 1)',
                  justifyContent: 'flex-start',
                  transform: `translateX(-${currentIndex * cardWidth}px)`,
                  width: `${ecosystemProjects.length * cardWidth}px`,
                }}
              >
                {ecosystemProjects.map((project, index) => {
                  const IconComponent = project.icon;
                  return (
                    <Box
                      key={project.name}
                      style={{ 
                        width: `${cardWidth}px`,
                        flexShrink: 0,
                        padding: '0 16px',
                        transition: 'all 0.5s cubic-bezier(0.4, 0, 0.2, 1)',
                        ...getCardStyle(index)
                      }}
                    >
                      <Card 
                        radius="md"
                        shadow="sm"
                        h="100%"
                        style={{
                          background: 'rgba(37, 38, 43, 0.3)',
                          border: '1px solid rgba(134, 142, 150, 0.5)',
                          transition: 'all 0.5s ease',
                          position: 'relative'
                        }}
                      >
                        <Stack justify="space-between" h="100%" align="center">
                          <Stack align="center" gap="md">
                            <Box 
                              style={{
                                width: '48px',
                                height: '48px',
                                background: 'linear-gradient(135deg, #868e96, #495057)',
                                borderRadius: '8px',
                                display: 'flex',
                                alignItems: 'center',
                                justifyContent: 'center',
                                color: '#e9ecef',
                                transition: 'transform 0.3s ease'
                              }}
                            >
                              <IconComponent size={24} />
                            </Box>
                            <Stack align="center" gap="xs">
                              <Title order={4} ta="center" className={classes.cardTitle}>
                                {project.name}
                              </Title>
                              <Text size="sm" ta="center" lh={1.4} className={classes.cardDescription}>
                                {project.description}
                              </Text>
                            </Stack>
                          </Stack>
                          
                          <Button 
                            component={Link}
                            href={project.href}
                            target="_blank"
                            size="sm"
                            radius="md"
                            variant="gradient"
                            gradient={{ from: 'gray.7', to: 'gray.6' }}
                            rightSection={<IconExternalLink size={14} />}
                            fullWidth
                            style={{
                              border: '1px solid rgba(173, 181, 189, 0.5)',
                              transition: 'all 0.3s ease'
                            }}
                          >
                            View Project
                          </Button>
                        </Stack>
                      </Card>
                    </Box>
                  );
                })}
              </Flex>
            </Box>

            {/* Dots Indicator */}
            <Group justify="center" gap="xs" mt="xl">
              {Array.from({ length: ecosystemProjects.length - 2 }, (_, index) => (
                <Box
                  key={index}
                  component="button"
                  onClick={() => setCurrentIndex(index)}
                  style={{
                    width: index === currentIndex ? '32px' : '8px',
                    height: '8px',
                    borderRadius: index === currentIndex ? '4px' : '50%',
                    background: index === currentIndex ? '#f783ac' : '#868e96',
                    border: 'none',
                    cursor: 'pointer',
                    transition: 'all 0.3s ease'
                  }}
                />
              ))}
            </Group>
          </Box>
        </Stack>
    </Container>
    </Box>
  );
}