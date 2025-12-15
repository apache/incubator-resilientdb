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

import { useSearchParams } from 'next/navigation';
import { Container, Paper, Text, Title } from '@mantine/core';
import { AuthProviders } from '@/components/AuthProviders';

function ErrorContent() {
  const searchParams = useSearchParams();
  const error = searchParams?.get('error');

  return (
    <Container size="xs" mt={40}>
      <Paper radius="md" p="xl" withBorder>
        <Title order={2} ta="center" mt="md" mb={30} c="red">
          Authentication Error
        </Title>
        <Text size="sm" ta="center">
          {error === 'OAuthSignin'
            ? 'An error occurred during sign in. Please try again.'
            : error || 'An unknown error occurred'}
        </Text>
      </Paper>
    </Container>
  );
}

export default function Error() {
  return (
    <AuthProviders>
      <ErrorContent />
    </AuthProviders>
  );
}
