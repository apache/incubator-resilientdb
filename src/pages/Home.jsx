/*global chrome*/
import logo from '../logo.png';
import '../App.css';
import CryptoJS from "crypto-js";
import { sendRequest } from '../client';
import Footer from "../components/Footer";
import { useAlert } from 'react-alert'
import IconButton from "@material-ui/core/IconButton";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";
import bcrypt from 'bcryptjs';
import React, { useEffect } from 'react';

function Home(props) {
  useEffect(() => {
    chrome.storage.sync.get(["store"], (res) => {
      console.log(res.store.publicKey);
      if(res.store.publicKey){
        props.setPublicKeyDisplay(true);
        props.setPublicKey(res.store.publicKey);
        props.setHash(res.store.hash);
        props.setEncryptedPrivateKey(res.store.encryptedPrivateKey);
        props.navigate("/login");
      }
    });
  });

  const alert = useAlert();
  props.setFooter("footer");

  const salt = bcrypt.genSaltSync(10);

  const createAccount = async () => {
    if(props.values.password===props.confirmValues.password){
      const query = `mutation {
        generateKeys {
          publicKey
          privateKey
        }
      }`

      chrome.storage.sync.clear(async function(){
          const result = await sendRequest(query).then(res => { 
          const getPublicKey = res.data.generateKeys.publicKey;
          props.setPublicKeyDisplay(true);
          props.setPublicKey(getPublicKey);
          props.setPrivateKey(res.data.generateKeys.privateKey);

          const phrase = CryptoJS.AES.encrypt(
            JSON.stringify(res.data.generateKeys.privateKey),
            props.values.password
          ).toString();
          props.setEncryptedPrivateKey(phrase);

          var hash = bcrypt.hashSync(props.values.password, salt);
          
          const store = {publicKey: props.publicKey, encryptedPrivateKey: props.encryptedPrivateKey, hash: hash};
          return store;
        }).then(store => {
          chrome.storage.sync.set({ store }, () => {
            /* Stored in local storage */
            console.log("test");
            props.navigate("/dashboard");
          });
        });
      });
    }
    else {
      alert.show("Passwords don't match.");
    }
    
  }

  const handleClickShowPassword = () => {
    props.setValues({ ...props.values, showPassword: !props.values.showPassword });
  };
  
  const handleMouseDownPassword = (event) => {
    event.preventDefault();
  };
  
  const handlePasswordChange = (prop) => (event) => {
    props.setValues({ ...props.values, [prop]: event.target.value });
  };

  const handleClickShowConfirmPassword = () => {
    props.setConfirmValues({ ...props.confirmValues, showPassword: !props.confirmValues.showPassword });
  };
  
  const handleMouseDownConfirmPassword = (event) => {
    event.preventDefault();
  };
  
  const handleConfirmPasswordChange = (prop) => (event) => {
    props.setConfirmValues({ ...props.confirmValues, [prop]: event.target.value });
  };
 

  return (
  <div className="App">
    <div className="mainContainer">
      <div className="cardHolder">
        <div className="header">
          <div className="heading center">Global-Scale Blockchain Fabric</div>
          <div className="stepHeading center">NexRes Wallet</div>
          <div className="logo">
            <img src={logo} alt="logo" />
          </div>
        </div>
      </div>

      <div className="paymentTop vcenter">
        <Input
          type={props.values.showPassword ? "text" : "password"}
          onChange={handlePasswordChange("password")}
          placeholder="Password"
          className="inputStyle"
          value={props.values.password}
          disableUnderline
          style={{ height: 50 }}
          endAdornment={
            <InputAdornment position="end">
              <IconButton
                onClick={handleClickShowPassword}
                onMouseDown={handleMouseDownPassword}
              >
                {props.values.showPassword ? <Visibility /> : <VisibilityOff />}
              </IconButton>
            </InputAdornment>
          }
        />
      </div>
      <div className="paymentBottom vcenter">
      <Input
          type={props.confirmValues.showPassword ? "text" : "password"}
          onChange={handleConfirmPasswordChange("password")}
          placeholder="Confirm Password"
          className="inputStyle"
          value={props.confirmValues.password}
          disableUnderline
          style={{ height: 50 }}
          endAdornment={
            <InputAdornment position="end">
              <IconButton
                onClick={handleClickShowConfirmPassword}
                onMouseDown={handleMouseDownConfirmPassword}
              >
                {props.confirmValues.showPassword ? <Visibility /> : <VisibilityOff />}
              </IconButton>
            </InputAdornment>
          }
        />
      </div>
    
      <div className="payment vcenter">
      <button className="buttonCreate center" onClick={createAccount}>Create Account</button>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Home;