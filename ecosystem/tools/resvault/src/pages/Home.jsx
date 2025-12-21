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


/*global chrome*/
import '../css/App.css';
import CryptoJS from "crypto-js";
import React, { useEffect, useContext } from 'react';
import { useNavigate } from "react-router-dom";
import splash from "../images/resvault.png";
import Lottie from 'react-lottie';
import versionData from '../data/version.json';
import { GlobalContext } from '../context/GlobalContext';

function Home() {
  const { setPublicKey, setPrivateKey } = useContext(GlobalContext);
  const navigate = useNavigate();

  const defaultOptions = {
    loop: true,
    autoplay: true,
    animationData: require('../images/bg.json'),
    rendererSettings: {
      preserveAspectRatio: 'xMidYMid slice',
      renderer: 'svg'  
    }
  };

  useEffect(() => {
    // Retrieve stored public and private keys
    chrome.storage.sync.get(["store"], (res) => {
      if (res.store && res.store.publicKey) {
        chrome.storage.local.get(["password"], (result) => {
          if (!result.password) {
            // If no password, navigate to login
            navigate("/login", { state: res.store });
          } else {
            // Decrypt private key and store both public and private keys
            const bytes = CryptoJS.AES.decrypt(res.store.encryptedPrivateKey, result.password);
            const privateKey = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
            setPublicKey(res.store.publicKey);
            setPrivateKey(privateKey);

            // Navigate to the dashboard
            navigate("/dashboard");
          }
        });
      }
    });
  }, [navigate, setPublicKey, setPrivateKey]);

  const handleSignup = () => {
    navigate("/signup");
  };

  return (
    <>
      <div className="lottie-background">
        <Lottie options={defaultOptions} height="100%" width="100%" />
      </div>
      <div className="page page--splash" data-page="splash">
        <div className="splash">
          <div className="splash__content">
            <div className="splash__logo">Res<strong>Vault</strong></div>
            <div className="splash__image"><img src={splash} alt="" title=""/></div>
            <div className="splash__text"></div>
            <div className="splash__buttons">
              <button className="button button--full button--main" onClick={handleSignup}>Signup</button>
            </div>
          </div>
        </div>
        <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>ResVault v{versionData.version}</p>
      </div>
    </>
  );
}

export default Home;