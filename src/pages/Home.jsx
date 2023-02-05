import logo from '../logo.png';
import '../App.css';
import React, { useState } from 'react';
import CryptoJS from "crypto-js";
import { sendRequest } from '../client';
import Footer from "../components/Footer";
import { useAlert } from 'react-alert'
import IconButton from "@material-ui/core/IconButton";
import InputLabel from "@material-ui/core/InputLabel";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";

function Home(props) {
  const alert = useAlert();

  const createAccount = async () => {
    if(props.values.password===props.confirmValues.password){
      const query = `mutation {
        generateKeys {
          publicKey
          privateKey
        }
      }`
    
      const res = await sendRequest(query);
      const getPublicKey = res.data.generateKeys.publicKey;
      props.setPublicKeyDisplay(true);
      props.setPublicKey(getPublicKey);
      const phrase = CryptoJS.AES.encrypt(
        JSON.stringify(res.data.generateKeys.privateKey),
        props.values.password
      ).toString();
      console.log(props.publicKey);
      console.log(phrase);

      const bytes = CryptoJS.AES.decrypt(phrase, props.values.password);
      const data = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
      console.log(data);
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

      
      {props.publicKeyDisplay &&
      <div>
      </div>
      }

      <Footer />
    </div>
  </div>
  )
}

export default Home;