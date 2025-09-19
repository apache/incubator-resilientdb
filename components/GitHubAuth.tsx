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
      <div
        style={{
          display: 'flex',
          alignItems: 'center',
          gap: 12,
          padding: 12,
          background: 'rgba(255,255,255,0.05)',
          backdropFilter: 'blur(8px)',
          border: '1px solid rgba(255,255,255,0.1)',
          borderRadius: 12,
        }}
      >
        <Avatar 
          src={user.avatar_url} 
          alt={user.login} 
          radius="xl"
          style={{
            border: '2px solid rgba(255,255,255,0.2)',
          }}
        />
        <div style={{ flex: 1 }}>
          <Text size="sm" c="rgba(255,255,255,0.9)" fw={400}>
            Signed in as <span style={{ color: '#00bfff', fontWeight: 500 }}>{user.name || user.login}</span>
          </Text>
        </div>
        <Button 
          onClick={handleSignOut} 
          size="xs"
          style={{
            background: 'rgba(255,255,255,0.05)',
            border: '1px solid rgba(255,255,255,0.1)',
            color: 'rgba(255,255,255,0.7)',
            borderRadius: 8,
            transition: 'all 200ms ease',
          }}
          onMouseEnter={(e) => {
            e.currentTarget.style.background = 'rgba(255, 77, 77, 0.1)';
            e.currentTarget.style.borderColor = 'rgba(255, 77, 77, 0.3)';
            e.currentTarget.style.color = '#ff4d4d';
          }}
          onMouseLeave={(e) => {
            e.currentTarget.style.background = 'rgba(255,255,255,0.05)';
            e.currentTarget.style.borderColor = 'rgba(255,255,255,0.1)';
            e.currentTarget.style.color = 'rgba(255,255,255,0.7)';
          }}
        >
          Sign Out
        </Button>
      </div>
    );
  }

  return (
    <Button
      leftSection={<IconBrandGithub size={16} />}
      onClick={handleSignIn}
      style={{
        background: 'rgba(255,255,255,0.05)',
        backdropFilter: 'blur(8px)',
        border: '1px solid rgba(255,255,255,0.1)',
        color: 'rgba(255,255,255,0.9)',
        borderRadius: 12,
        padding: '12px 20px',
        fontWeight: 400,
        transition: 'all 200ms ease',
        position: 'relative',
        overflow: 'hidden',
      }}
      onMouseEnter={(e) => {
        e.currentTarget.style.background = 'rgba(0, 191, 255, 0.1)';
        e.currentTarget.style.borderColor = 'rgba(0, 191, 255, 0.3)';
        e.currentTarget.style.color = '#00bfff';
        e.currentTarget.style.transform = 'translateY(-1px)';
        e.currentTarget.style.boxShadow = '0 4px 12px rgba(0, 191, 255, 0.2)';
      }}
      onMouseLeave={(e) => {
        e.currentTarget.style.background = 'rgba(255,255,255,0.05)';
        e.currentTarget.style.borderColor = 'rgba(255,255,255,0.1)';
        e.currentTarget.style.color = 'rgba(255,255,255,0.9)';
        e.currentTarget.style.transform = 'translateY(0)';
        e.currentTarget.style.boxShadow = 'none';
      }}
    >
      <div
        style={{
          position: 'absolute',
          top: 0,
          left: 0,
          right: 0,
          height: 1,
          background: 'linear-gradient(90deg, transparent, rgba(255,255,255,0.2), transparent)',
          borderRadius: '12px 12px 0 0',
        }}
      />
      <span style={{ position: 'relative', zIndex: 1 }}>
        Sign in with GitHub to Comment
      </span>
    </Button>
  );
} 