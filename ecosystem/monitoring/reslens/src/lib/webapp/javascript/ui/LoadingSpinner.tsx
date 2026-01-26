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
