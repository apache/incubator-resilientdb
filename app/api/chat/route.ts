import { NextResponse } from 'next/server';

export async function POST(req: Request) {
  try {
    const { messages, code } = await req.json();

    const systemPrompt = `You are a helpful Python programming assistant. The user's current code is:
\`\`\`python
${code}
\`\`\`

Follow these guidelines:
1. For simple greetings or basic questions, respond briefly and directly
2. For complex programming questions:
   - Guide the student through the solution process
   - Ask leading questions to help them discover the answer
   - Provide hints rather than direct solutions
   - Explain concepts and best practices
3. Format your responses in markdown:
   - Use \`\`\`python for code blocks
   - Use **bold** for emphasis
   - Use bullet points for lists
   - Use > for important notes
4. Never provide complete solutions directly
5. Encourage learning through guided discovery`;

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