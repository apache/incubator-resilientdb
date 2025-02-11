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
*
*/


/*global chrome*/
import '../css/App.css';
import CryptoJS from "crypto-js";  // Changed to default import
import IconButton from "@material-ui/core/IconButton";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";
import React, { useContext, useState, useEffect } from 'react';
import Lottie from 'react-lottie';
import { useNavigate } from "react-router-dom";
import versionData from '../data/version.json';
import { GlobalContext } from '../context/GlobalContext';

function Login() {
  const {
    loginValues,
    setLoginValues,
    setPublicKey,
    setPrivateKey,
    setIsAuthenticated,
    setKeyPairs,
    setSelectedKeyPairIndex,
    setStoredPassword
  } = useContext(GlobalContext);
  const navigate = useNavigate();
  const [showPasswordErrorModal, setShowPasswordErrorModal] = useState(false);

  const defaultOptions = {
    loop: true,
    autoplay: true,
    animationData: require('../images/signup.json'),
    rendererSettings: {
      preserveAspectRatio: 'xMidYMid slice',
      renderer: 'svg'
    }
  };

  const removeAccount = async () => {
    chrome.storage.sync.clear(function() {});
    chrome.storage.local.clear(function() {});
    localStorage.removeItem('nets');
    setIsAuthenticated(false);
    navigate("/");
  };

  const loginAccount = async (e) => {
    e.preventDefault();
    chrome.storage.sync.get(['store', 'encryptedKeyPairs'], (res) => {
      if (res.store && CryptoJS.SHA256(loginValues.password).toString(CryptoJS.enc.Hex) === res.store.hash) {
        try {
          // Decrypt private key from store
          const privateKeyBytes = CryptoJS.AES.decrypt(res.store.encryptedPrivateKey, loginValues.password);
          const decryptedPrivateKey = privateKeyBytes.toString(CryptoJS.enc.Utf8);
          const publicKey = res.store.publicKey;

          // Check if encryptedKeyPairs exist
          if (!res.encryptedKeyPairs) {
            console.error('No encryptedKeyPairs found in storage.');
            setShowPasswordErrorModal(true);
            return;
          }

          // Load key pairs
          const encryptedKeyPairs = res.encryptedKeyPairs;
          const decryptedKeyPairsBytes = CryptoJS.AES.decrypt(encryptedKeyPairs, loginValues.password);
          const decryptedKeyPairsString = decryptedKeyPairsBytes.toString(CryptoJS.enc.Utf8);

          let decryptedKeyPairs;
          try {
            decryptedKeyPairs = JSON.parse(decryptedKeyPairsString);
          } catch (parseError) {
            console.error('Error parsing decrypted key pairs:', parseError, 'decryptedKeyPairsString:', decryptedKeyPairsString);
            setShowPasswordErrorModal(true);
            return;
          }

          // Ensure decryptedKeyPairs is an array
          if (!Array.isArray(decryptedKeyPairs)) {
            console.error('Decrypted key pairs is not an array:', decryptedKeyPairs);
            setShowPasswordErrorModal(true);
            return;
          }

          // Update keyPairs and set keys
          setKeyPairs(decryptedKeyPairs);
          // Find the index of the key pair matching the publicKey
          const keyPairIndex = decryptedKeyPairs.findIndex(kp => kp.publicKey === publicKey);
          if (keyPairIndex !== -1) {
            setSelectedKeyPairIndex(keyPairIndex);
            setPublicKey(decryptedKeyPairs[keyPairIndex].publicKey);
            setPrivateKey(decryptedKeyPairs[keyPairIndex].privateKey);
          } else {
            // If not found, default to first key pair
            setSelectedKeyPairIndex(0);
            setPublicKey(decryptedKeyPairs[0].publicKey);
            setPrivateKey(decryptedKeyPairs[0].privateKey);
          }

          // Store password in chrome.storage.local and set storedPassword in context
          chrome.storage.local.set({ password: loginValues.password }, () => {
            setStoredPassword(loginValues.password);
            setIsAuthenticated(true);
            navigate("/dashboard");
          });
        } catch (err) {
          console.error('Error during login:', err);
          setShowPasswordErrorModal(true);
        }
      } else {
        setShowPasswordErrorModal(true);
      }
    });
  };

  const handleClickShowPassword = () => {
    setLoginValues({ ...loginValues, showPassword: !loginValues.showPassword });
  };

  const handleMouseDownPassword = (event) => {
    event.preventDefault();
  };

  const handlePasswordChange = (prop) => (event) => {
    setLoginValues({ ...loginValues, [prop]: event.target.value });
  };

  const closeModal = () => {
    setShowPasswordErrorModal(false);
  };

  // Reset loginValues when the component mounts
  useEffect(() => {
    setLoginValues({ password: '', showPassword: false });
  }, [setLoginValues]);

  return (
    <>
      <div className="lottie-background">
        <Lottie options={defaultOptions} height="100%" width="100%" />
      </div>
      <div className="page page--login" data-page="login">
        <div className="login">
          <div className="login__content">
            <h2 className="login__title">Login</h2>
            <div className="login-form">
              <form id="LoginForm" onSubmit={loginAccount}>
                <div className="login-form__row">
                  <label className="login-form__label">Password</label>
                  <Input
                    type={loginValues.showPassword ? "text" : "password"}
                    onChange={handlePasswordChange("password")}
                    placeholder="Password"
                    className="login-form__input"
                    value={loginValues.password}
                    style={{ color: 'white', width: '100%' }}
                    disableUnderline
                    required
                    endAdornment={
                      <InputAdornment position="end">
                        <IconButton
                          onClick={handleClickShowPassword}
                          onMouseDown={handleMouseDownPassword}
                          style={{ color: 'white' }}
                        >
                          {loginValues.showPassword ? <Visibility /> : <VisibilityOff />}
                        </IconButton>
                      </InputAdornment>
                    }
                  />
                </div>
                <div className="login-form__row">
                  <button
                    disabled={!loginValues.password}
                    className="login-form__submit button button--main button--full"
                    type="submit"
                  >
                    Login
                  </button>
                </div>
              </form>
              <div className="login-form__bottom">
                <p>
                  Delete current account? <br />
                  <button
                    style={{
                      color: '#47e7ce',
                      fontWeight: '600',
                      fontSize: '1.2rem',
                      background: 'transparent',
                      border: 'none',
                      outline: 'none',
                      cursor: 'pointer',
                    }}
                    onClick={removeAccount}
                  >
                    Remove Account
                  </button>
                </p>
              </div>
              <p
                className="bottom-navigation"
                style={{
                  backgroundColor: 'transparent',
                  display: 'flex',
                  justifyContent: 'center',
                  textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)',
                  color: 'rgb(255, 255, 255, 0.5)',
                  fontSize: '9px',
                }}
              >
                ResVault v{versionData.version}
              </p>
            </div>
          </div>
        </div>

        {showPasswordErrorModal && (
          <div className="modal-overlay">
            <div className="modal">
              <h2>Password Incorrect!</h2>
              <p>Please check your password and try again.</p>
              <button className="button button--main button--full" onClick={closeModal}>
                OK
              </button>
            </div>
          </div>
        )}
      </div>
    </>
  );
}

export default Login;
