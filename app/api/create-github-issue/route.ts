import { NextResponse } from 'next/server';
import { Octokit } from '@octokit/rest';
import { getServerSession } from 'next-auth';
import { authOptions } from '../auth/[...nextauth]/route';

export async function POST(request: Request) {
  try {
    const session = await getServerSession(authOptions);

    if (!session?.accessToken) {
      return NextResponse.json(
        { error: 'Unauthorized - Please sign in with GitHub' },
        { status: 401 }
      );
    }

    const { title, body, repoOwner, repoName, labels } = await request.json();

    // Validate required fields
    if (!title || !body || !repoOwner || !repoName) {
      return NextResponse.json(
        { error: 'Title, body, repository owner, and repository name are required' },
        { status: 400 }
      );
    }

    // Initialize Octokit with the user's access token
    const octokit = new Octokit({
      auth: session.accessToken,
    });

    try {
      // Create the issue using GitHub API with the user's token
      const response = await octokit.issues.create({
        owner: repoOwner,
        repo: repoName,
        title,
        body: `${body}\n\n---\n_Posted via Beacon Documentation_`,
        labels: labels || ['user-feedback', 'documentation'],
      });

      return NextResponse.json({
        success: true,
        issueUrl: response.data.html_url,
        issueNumber: response.data.number,
      });
    } catch (githubError: any) {
      // Handle specific GitHub API errors
      const errorMessage = githubError.response?.data?.message || 'Failed to create GitHub issue';
      return NextResponse.json(
        { error: errorMessage },
        { status: githubError.status || 500 }
      );
    }
  } catch (error) {
    console.error('Error processing request:', error);
    return NextResponse.json(
      { error: 'Internal server error' },
      { status: 500 }
    );
  }
} 