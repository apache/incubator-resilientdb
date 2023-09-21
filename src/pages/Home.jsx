/*global chrome*/
import '../css/App.css';
import CryptoJS from "crypto-js";
import React, { useEffect } from 'react';
import { useLocation } from "react-router-dom";
import splash from "../images/splash.svg";

function Home(props) {
  useEffect(() => {
    chrome.storage.sync.get(["store"], (res) => {
      if(res.store.publicKey){
        chrome.storage.local.get(["password"], (result) => {
          if(!result.password) {
            props.navigate("/login", {state: res.store} );
          }
          else {
            const bytes = CryptoJS.AES.decrypt(res.store.encryptedPrivateKey, result.password.password);
            const data = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
            const store = res.store;
            store.privateKey = data;
            props.navigate("/dashboard", {state: store});
          }
        })
      }
    });
  });
  const location = useLocation();

  const signup = async () => {
    props.navigate("/signup", {state: location.state});
  }

  return (
    <div className="page page--splash" data-page="splash">
    <div className="splash">
    <div className="splash__content">
      <div className="splash__logo">Res<strong>Vault</strong></div>
      <div className="splash__image"><img src={splash} alt="" title=""/></div>
      <div className="splash__text"></div>
      <div className="splash__buttons">
        <button className="button button--full button--main" onClick={signup}>Signup</button>
      </div>
    </div>
    </div>
    <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>ResVault v0.0.1 Alpha Release</p>
    </div>
  )
}

export default Home;