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

import { NextResponse } from 'next/server';

export async function POST(req: Request) {
  try {
    const { messages, code } = await req.json();

    const systemPrompt = `You are a helpful assistant for ResilientDB documentation. You have access to comprehensive documentation about ResilientDB, a distributed database system.

Your role is to:
1. Answer questions about ResilientDB concepts, architecture, and usage
2. Provide accurate information based on the documentation
3. Explain complex concepts in clear, understandable terms
4. Offer practical examples and use cases when relevant
5. Reference specific documentation sections when helpful

Guidelines:
- Be accurate and factual in your responses
- Use markdown formatting for better readability
- Provide detailed explanations for technical concepts
- Include relevant examples from the documentation
- If you're unsure about something, say so rather than guessing
- Focus on helping users understand ResilientDB better

Format your responses with:
- **Bold** for emphasis
- \`code blocks\` for technical terms and code
- Bullet points for lists
- > for important notes or tips
- Clear headings for different sections

Always aim to be helpful, accurate, and educational.`;

    const response = await fetch('https://api.deepseek.com/v1/chat/completions', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'Authorization': `Bearer ${process.env.DEEPSEEK_API_KEY}`,
      },
      body: JSON.stringify({
        model: 'deepseek-chat',
        messages: [
          { role: 'system', content: systemPrompt },
          ...messages,
        ],
        stream: true,
      }),
    });

    if (!response.ok) {
      return new Response('Error from DeepSeek API', { status: response.status });
    }

    // Create a TransformStream to process the response
    const encoder = new TextEncoder();
    const decoder = new TextDecoder();

    const stream = new ReadableStream({
      async start(controller) {
        const reader = response.body?.getReader();
        if (!reader) {
          controller.close();
          return;
        }

        try {
          while (true) {
            const { done, value } = await reader.read();
            if (done) break;

            const chunk = decoder.decode(value);
            const lines = chunk.split('\n').filter(line => line.trim() !== '');

            for (const line of lines) {
              if (line.startsWith('data: ')) {
                const data = line.slice(6);
                if (data === '[DONE]') continue;

                try {
                  const parsed = JSON.parse(data);
                  const content = parsed.choices[0]?.delta?.content || '';
                  if (content) {
                    controller.enqueue(encoder.encode(content));
                  }
                } catch (e) {
                  console.error('Error parsing JSON:', e);
                }
              }
            }
          }
        } catch (e) {
          console.error('Stream processing error:', e);
          controller.error(e);
        } finally {
          controller.close();
        }
      },
    });

    return new Response(stream, {
      headers: {
        'Content-Type': 'text/plain; charset=utf-8',
        'Transfer-Encoding': 'chunked',
      },
    });
  } catch (error) {
    console.error('Chat API error:', error);
    return NextResponse.json(
      { error: 'Failed to process chat request' },
      { status: 500 }
    );
  }
} 