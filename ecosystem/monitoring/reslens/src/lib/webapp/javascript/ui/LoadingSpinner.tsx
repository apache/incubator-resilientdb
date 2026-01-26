import React from 'react';

interface LoadingSpinnerProps {
  className?: string;
  size?: string;
}

export default function LoadingSpinner({
  className,
  size = '20px',
}: LoadingSpinnerProps) {
  const spinnerStyle: React.CSSProperties = {
    width: size,
    height: size,
    border: '2px solid rgba(255,255,255,0.3)',
    borderTop: '2px solid rgba(255,255,255,0.6)',
    borderRadius: '50%',
    animation: 'spin 1s linear infinite',
    display: 'inline-block',
  };

  // Add keyframes for the spin animation
  React.useEffect(() => {
    if (!document.getElementById('spinner-keyframes')) {
      const style = document.createElement('style');
      style.id = 'spinner-keyframes';
      style.textContent = `
        @keyframes spin {
          0% { transform: rotate(0deg); }
          100% { transform: rotate(360deg); }
        }
      `;
      document.head.appendChild(style);
    }
  }, []);

  return (
    <span role="progressbar" className={className}>
      <div style={spinnerStyle} />
    </span>
  );
}
