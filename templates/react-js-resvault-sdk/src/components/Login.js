import React, { useEffect, useRef, useState } from 'react';
import ResVaultSDK from 'resvault-sdk';
import '../App.css';
import resvaultLogo from '../assets/images/resilientdb.svg';
import NotificationModal from './NotificationModal';
import { v4 as uuidv4 } from 'uuid';
import lottie from 'lottie-web';
import animation from '../assets/images/animation.json';

const Login = ({ onLogin }) => {
  const sdkRef = useRef(null);
  const [showModal, setShowModal] = useState(false);
  const [modalTitle, setModalTitle] = useState('');
  const [modalMessage, setModalMessage] = useState('');

  if (!sdkRef.current) {
    sdkRef.current = new ResVaultSDK();
  }

  const animationContainer = useRef(null);

  useEffect(() => {
    if (animationContainer.current) {
      const instance = lottie.loadAnimation({
        container: animationContainer.current,
        renderer: 'svg',
        loop: true,
        autoplay: true,
        animationData: animation,
      });

      const observer = new IntersectionObserver((entries) => {
        entries.forEach((entry) => {
          if (entry.isIntersecting) {
            instance.play();
          } else {
            instance.pause();
          }
        });
      });

      observer.observe(animationContainer.current);

      return () => {
        instance.destroy();
        observer.disconnect();
      };
    } else {
      console.error('Animation container is not defined');
    }
  }, []);

  useEffect(() => {
    const sdk = sdkRef.current;
    if (!sdk) return;

    const messageHandler = (event) => {
      const message = event.data;

      if (
        message &&
        message.type === 'FROM_CONTENT_SCRIPT' &&
        message.data &&
        message.data.success !== undefined
      ) {
        if (message.data.success) {
          const token = uuidv4();
          sessionStorage.setItem('token', token);
          onLogin(token);
        }
      } else if (
        message &&
        message.type === 'FROM_CONTENT_SCRIPT' &&
        message.data &&
        message.data === 'error'
      ) {
        setModalTitle('Authentication Failed');
        setModalMessage(
          'Please connect ResVault to this ResilientApp and try again.'
        );
        setShowModal(true);
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
          <h2 className="heading">Resilient App</h2>

          <div ref={animationContainer} className="animation-container"></div>

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