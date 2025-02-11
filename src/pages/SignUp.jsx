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
import CryptoJS from "crypto-js";
import IconButton from "@material-ui/core/IconButton";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";
import PasswordStrengthBar from 'react-password-strength-bar';
import backicon from "../images/icons/arrow-back.svg";
import React, { useContext, useState, useEffect } from 'react'; // Note the import of useEffect
import Lottie from 'react-lottie';
import versionData from '../data/version.json';
import { useNavigate } from 'react-router-dom';
import { GlobalContext } from '../context/GlobalContext';

function SignUp() {
    const {
        values,
        setValues,
        confirmValues,
        setConfirmValues,
        generateKeyPair,
        setIsAuthenticated,
        setStoredPassword,
        storedPassword,
    } = useContext(GlobalContext);
    const navigate = useNavigate();
    const [showPasswordMismatchModal, setShowPasswordMismatchModal] = useState(false);
    const [shouldGenerateKeyPair, setShouldGenerateKeyPair] = useState(false);

    const defaultOptions = {
        loop: true,
        autoplay: true,
        animationData: require('../images/signup.json'),
        rendererSettings: {
            preserveAspectRatio: 'xMidYMid slice',
            renderer: 'svg'
        }
    };

    const back = () => {
        navigate("/");
    };

    const buttonStyle = {
        backgroundColor: 'transparent',
        cursor: 'pointer',
        border: 'none',
        boxShadow: 'none',
        outline: 'none',
    };

    const createAccount = async (e) => {
        e.preventDefault();
        if (values.password === confirmValues.password) {
            const password = values.password;
            // Store password
            chrome.storage.local.set({ password }, () => {
                setStoredPassword(password);
                setShouldGenerateKeyPair(true);
            });
        } else {
            setShowPasswordMismatchModal(true);
        }
    };

    useEffect(() => {
        if (shouldGenerateKeyPair && storedPassword) {
            generateKeyPair(() => {
                setIsAuthenticated(true);
                navigate("/dashboard");
            });
            setShouldGenerateKeyPair(false);
        }
    }, [shouldGenerateKeyPair, storedPassword, generateKeyPair, setIsAuthenticated, navigate]);

    const handleClickShowPassword = () => {
        setValues({ ...values, showPassword: !values.showPassword });
    };

    const handleMouseDownPassword = (event) => {
        event.preventDefault();
    };

    const handlePasswordChange = (prop) => (event) => {
        setValues({ ...values, [prop]: event.target.value });
    };

    const handleClickShowConfirmPassword = () => {
        setConfirmValues({ ...confirmValues, showPassword: !confirmValues.showPassword });
    };

    const handleMouseDownConfirmPassword = (event) => {
        event.preventDefault();
    };

    const handleConfirmPasswordChange = (prop) => (event) => {
        setConfirmValues({ ...confirmValues, [prop]: event.target.value });
    };

    const closeModal = () => {
        setShowPasswordMismatchModal(false);
    };

    return (
        <>
            <div className="lottie-background">
                <Lottie options={defaultOptions} height="100%" width="100%" />
            </div>
            <div className="page page--login" data-page="login">
                <header className="header header--fixed">
                    <div className="header__inner">
                        <div className="header__icon">
                            <button onClick={back} style={buttonStyle}>
                                <img src={backicon} alt="back" />
                            </button>
                        </div>
                    </div>
                </header>

                <div className="login">
                    <div className="login__content">
                        <h2 className="login__title">Create an account</h2>
                        <div className="login-form">
                            <form id="LoginForm" method="post" onSubmit={createAccount}>
                                <div className="login-form__row">
                                    <label className="login-form__label">Password</label>
                                    <Input
                                        type={values.showPassword ? "text" : "password"}
                                        onChange={handlePasswordChange("password")}
                                        placeholder="Password"
                                        className="login-form__input"
                                        value={values.password}
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
                                                    {values.showPassword ? <Visibility /> : <VisibilityOff />}
                                                </IconButton>
                                            </InputAdornment>
                                        }
                                    />
                                </div>
                                <div className="login-form__row">
                                    <label className="login-form__label">Confirm Password</label>
                                    <Input
                                        type={confirmValues.showPassword ? "text" : "password"}
                                        onChange={handleConfirmPasswordChange("password")}
                                        placeholder="Confirm Password"
                                        className="login-form__input"
                                        value={confirmValues.password}
                                        style={{ color: 'white', width: '100%' }}
                                        disableUnderline
                                        required
                                        endAdornment={
                                            <InputAdornment position="end">
                                                <IconButton
                                                    onClick={handleClickShowConfirmPassword}
                                                    onMouseDown={handleMouseDownConfirmPassword}
                                                    style={{ color: 'white' }}
                                                >
                                                    {confirmValues.showPassword ? <Visibility /> : <VisibilityOff />}
                                                </IconButton>
                                            </InputAdornment>
                                        }
                                    />
                                </div>
                                <PasswordStrengthBar password={values.password} />
                                <div className="login-form__row">
                                    <button
                                        disabled={!values.password || !confirmValues.password}
                                        className="login-form__submit button button--main button--full"
                                        id="submit"
                                        type="submit"
                                    >
                                        Create Account
                                    </button>
                                </div>
                            </form>
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

                {showPasswordMismatchModal && (
                    <div className="modal-overlay">
                        <div className="modal">
                            <h2>Passwords do not match!</h2>
                            <p>Please make sure both passwords are the same.</p>
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

export default SignUp;
