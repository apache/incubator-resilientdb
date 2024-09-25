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
    const { loginValues, setLoginValues, setPublicKey, setPrivateKey } = useContext(GlobalContext);
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
        localStorage.removeItem('nets');
        navigate("/");
    };

    useEffect(() => {
        chrome.storage.local.get(['password'], (result) => {
            if (result.password) {
                chrome.storage.sync.get(['store'], (res) => {
                    if (res.store) {
                        setPublicKey(res.store.publicKey);
                        setPrivateKey(res.store.encryptedPrivateKey);
                        navigate("/dashboard");
                    }
                });
            }
        });
    }, [navigate, setPublicKey, setPrivateKey]);

    const loginAccount = async (e) => {
        e.preventDefault();
        chrome.storage.sync.get(['store'], (res) => {
            if (res.store && CryptoJS.SHA256(loginValues.password).toString(CryptoJS.enc.Hex) === res.store.hash) {
                try {
                    const bytes = CryptoJS.AES.decrypt(res.store.encryptedPrivateKey, loginValues.password);
                    const decryptedPrivateKey = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
                    setPublicKey(res.store.publicKey);
                    setPrivateKey(decryptedPrivateKey);
                    
                    chrome.storage.local.set({ password: { password: loginValues.password } }, () => {});
                    navigate("/dashboard");
                } catch (err) {
                    setShowPasswordErrorModal(true);  // Show modal if decryption fails
                }
            } else {
                setShowPasswordErrorModal(true);  // Show modal if password is incorrect
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
                                        style={{color: 'white', width: '100%'}}
                                        disableUnderline
                                        required
                                        endAdornment={
                                            <InputAdornment position="end">
                                                <IconButton
                                                    onClick={handleClickShowPassword}
                                                    onMouseDown={handleMouseDownPassword}
                                                    style={{color: 'white'}}
                                                >
                                                    {loginValues.showPassword ? <Visibility /> : <VisibilityOff />}
                                                </IconButton>
                                            </InputAdornment>
                                        }
                                    />
                                </div>
                                <div className="login-form__row">
                                    <button disabled={!loginValues.password} className="login-form__submit button button--main button--full" type="submit">
                                        Login
                                    </button>
                                </div>
                            </form>
                            <div className="login-form__bottom">
                                <p>
                                    Delete current account? <br />
                                    <button style={{color: '#47e7ce', fontWeight: '600', fontSize: '1.2rem', background: 'transparent', border: 'none', outline: 'none', cursor: 'pointer'}} onClick={removeAccount}>
                                        Remove Account
                                    </button>
                                </p>
                            </div>
                            <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>
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