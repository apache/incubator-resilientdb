/**
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