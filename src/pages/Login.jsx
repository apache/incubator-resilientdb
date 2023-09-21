/*global chrome*/
import '../css/App.css';
import CryptoJS from "crypto-js";
import IconButton from "@material-ui/core/IconButton";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";
import { useLocation } from "react-router-dom";
import React from 'react';

function Login(props) {
    const location = useLocation();

    const removeAccount = async () => {
        chrome.storage.sync.clear(function(){
          /*Cleared storage*/
        });
        props.navigate("/");
    }

    const loginAccount = async () => {
    if(CryptoJS.SHA256(props.loginValues.password).toString(CryptoJS.enc.Hex) === location.state.hash){
        try {
        const bytes = CryptoJS.AES.decrypt(location.state.encryptedPrivateKey, props.loginValues.password);
        const data = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
        const store = location.state;
        var password = {password: props.loginValues.password};
        store.privateKey = data;
        chrome.storage.local.set({ password }, () => {});
        props.navigate("/dashboard", {state: store});
        }
        catch(err) {
        props.navigate("/login");
        }
        
    }
    else {
        props.navigate("/login");
    }
    }

    const handleClickShowPassword = () => {
        props.setLoginValues({ ...props.loginValues, showPassword: !props.loginValues.showPassword });
      };
      
      const handleMouseDownPassword = (event) => {
        event.preventDefault();
      };
      
      const handlePasswordChange = (prop) => (event) => {
        props.setLoginValues({ ...props.loginValues, [prop]: event.target.value });
      };

    return (
        <div className="page page--login" data-page="login">
            <div className="login">
            <div className="login__content">	
                <h2 className="login__title">Login</h2>
                    <div className="login-form">
                        <form id="LoginForm" method="post" action="main.html">
                            <div className="login-form__row">
                                <label className="login-form__label">Password</label>
                                <Input
                                    type={props.loginValues.showPassword ? "text" : "password"}
                                    onChange={handlePasswordChange("password")}
                                    placeholder="Password"
                                    className="login-form__input"
                                    value={props.loginValues.password}
                                    style={{color: 'white', width: '100%'}}
                                    disableUnderline
                                    endAdornment={
                                        <InputAdornment position="end">
                                        <IconButton
                                            onClick={handleClickShowPassword}
                                            onMouseDown={handleMouseDownPassword}
                                            style={{color: 'white'}}
                                        >
                                            {props.loginValues.showPassword ? <Visibility /> : <VisibilityOff />}
                                        </IconButton>
                                        </InputAdornment>
                                    }
                                    />
                            </div>
                            <div className="login-form__row">
                                <button className="login-form__submit button button--main button--full" id="submit" onClick={loginAccount}>
                                    Login
                                </button>
                            </div>
                        </form>
                            
                        <div className="login-form__bottom">
                            <p>Delete current account? <br /><button style={{color: '#47e7ce', fontWeight: '600', fontSize: '1.2rem', background: 'transparent', border: 'none', 'outline': 'none', 'cursor': 'pointer'}} onClick={removeAccount}>Remove Account</button></p>
                        </div>
                        <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>ResVault v0.0.1 Alpha Release</p>
                    </div>
                </div>
            </div>
        </div>
  )
}

export default Login;