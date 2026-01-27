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
