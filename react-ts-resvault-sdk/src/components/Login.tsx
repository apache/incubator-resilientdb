// src/components/Login.tsx
import React, { useEffect, useRef, useState } from 'react';
import ResVaultSDK from 'resvault-sdk';
import '../App.css';
import resvaultLogo from '../assets/images/resilientdb.svg';
import NotificationModal from './NotificationModal';
import { v4 as uuidv4 } from 'uuid';

interface LoginProps {
  onLogin: (token: string) => void;
}

const Login: React.FC<LoginProps> = ({ onLogin }) => {
  const sdkRef = useRef<ResVaultSDK | null>(null);
  const [showModal, setShowModal] = useState<boolean>(false);
  const [modalTitle, setModalTitle] = useState<string>('');
  const [modalMessage, setModalMessage] = useState<string>('');

  if (!sdkRef.current) {
    sdkRef.current = new ResVaultSDK();
  }

  useEffect(() => {
    const sdk = sdkRef.current;
    if (!sdk) return;

    const messageHandler = (event: MessageEvent) => {
      const message = event.data;
      console.log('Received message:', message);

      if (message && message.type === 'FROM_CONTENT_SCRIPT') {
        if (message.data && message.data.success !== undefined) {
          if (message.data.success) {
            console.log('Authentication successful:', message.data);
            // Generate a unique token using uuid
            const token = uuidv4();
            sessionStorage.setItem('token', token);
            // Directly call onLogin without showing the success modal
            onLogin(token);
          } else {
            console.error('Authentication failed:', message.data.error || message.data.errors);
            setModalTitle('Authentication Failed');
            setModalMessage(
              'Authentication failed: ' +
                (message.data.error || JSON.stringify(message.data.errors))
            );
            setShowModal(true);
          }
        }
      }
    };

    sdk.addMessageListener(messageHandler);

    return () => {
      sdk.removeMessageListener(messageHandler);
    };
  }, [onLogin]);

  const handleAuthentication = () => {
    if (sdkRef.current) {
      sdkRef.current.sendMessage({
        type: 'login',
        direction: 'login',
      });
    } else {
      console.error('SDK is not initialized');
      setModalTitle('Error');
      setModalMessage('SDK is not initialized.');
      setShowModal(true);
    }
  };

  const handleCloseModal = () => setShowModal(false);

  return (
    <>
      <div className="page-container">
        <div className="form-container">
          {/* Center-aligned heading */}
          <h2 className="heading">ResilientDB Demo App</h2>

          {/* Authenticate Button */}
          <div className="form-group text-center mb-4">
            <label className="signin-label">Sign In Via</label>
            <button
              type="button"
              className="btn btn-secondary oauth-button"
              onClick={handleAuthentication}
            >
              <div className="logoBox">
                <img src={resvaultLogo} alt="ResVault" className="oauth-logo" />
              </div>
              <span className="oauth-text">ResVault</span>
            </button>
          </div>
        </div>
      </div>

      {/* Notification Modal for Errors */}
      <NotificationModal
        show={showModal}
        title={modalTitle}
        message={modalMessage}
        onClose={handleCloseModal}
      />
    </>
  );
};

export default Login;