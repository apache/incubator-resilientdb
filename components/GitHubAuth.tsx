'use client';

import { useState, useEffect } from 'react';
import { Button, Text, Group, Avatar } from '@mantine/core';
import { IconBrandGithub } from '@tabler/icons-react';
import { signIn } from 'next-auth/react';

interface GitHubUser {
  login: string;
  avatar_url: string;
  name: string | null;
}

interface GitHubAuthProps {
  onAuthStateChange: (user: GitHubUser | null) => void;
  returnUrl?: string;
}

export function GitHubAuth({ onAuthStateChange, returnUrl }: GitHubAuthProps) {
  const [user, setUser] = useState<GitHubUser | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    // Check if user is already authenticated
    checkAuthStatus();
  }, []);

  const checkAuthStatus = async () => {
    try {
      const response = await fetch('/api/auth/session');
      const data = await response.json();
      
      if (data.user) {
        setUser(data.user);
        onAuthStateChange(data.user);
      }
    } catch (error) {
      console.error('Error checking auth status:', error);
    } finally {
      setLoading(false);
    }
  };

  const handleSignIn = () => {
    signIn('github', { callbackUrl: returnUrl || window.location.href });
  };

  const handleSignOut = async () => {
    try {
      await fetch('/api/auth/signout', { method: 'POST' });
      setUser(null);
      onAuthStateChange(null);
    } catch (error) {
      console.error('Error signing out:', error);
    }
  };

  if (loading) {
    return null;
  }

  if (user) {
    return (
      <Group>
        <Avatar src={user.avatar_url} alt={user.login} radius="xl" />
        <Text size="sm">
          Signed in as <strong>{user.name || user.login}</strong>
        </Text>
        <Button variant="subtle" onClick={handleSignOut} size="sm">
          Sign Out
        </Button>
      </Group>
    );
  }

  return (
    <Button
      leftSection={<IconBrandGithub size={16} />}
      onClick={handleSignIn}
      variant="outline"
    >
      Sign in with GitHub to Comment
    </Button>
  );
} 