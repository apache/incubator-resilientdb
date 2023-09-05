/*global chrome*/
import logo from '../logo.svg';
import '../App.css';
import CryptoJS from "crypto-js";
import { useAlert } from 'react-alert'
import IconButton from "@material-ui/core/IconButton";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";
import DeleteForeverIcon from '@mui/icons-material/DeleteForever';
import bcrypt from 'bcryptjs';
import { useLocation } from "react-router-dom";

function Login(props) {
  const alert = useAlert();
  const location = useLocation();

  const removeAccount = async () => {
    chrome.storage.sync.clear(function(){
      /*Cleared storage*/
    });
    props.navigate("/");
  }

  const inputStyles = {
    backgroundColor: 'rgba(255, 255, 255, 0.8)',
    height: 50
  };

  const loginAccount = async () => {
    if(bcrypt.compareSync(props.loginValues.password, location.state.hash)){
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
        alert.show("Incorrect password.");
      }
      
    }
    else {
      alert.show("Incorrect password.");
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
          type={props.loginValues.showPassword ? "text" : "password"}
          onChange={handlePasswordChange("password")}
          placeholder="Password"
          className="inputStyle"
          value={props.loginValues.password}
          disableUnderline
          style={inputStyles}
          endAdornment={
            <InputAdornment position="end">
              <IconButton
                onClick={handleClickShowPassword}
                onMouseDown={handleMouseDownPassword}
              >
                {props.loginValues.showPassword ? <Visibility /> : <VisibilityOff />}
              </IconButton>
            </InputAdornment>
          }
        />
      </div>
    
      <div className="paymentBottom vcenter">
        <button className="buttonCreate center" onClick={loginAccount}>Login</button>
      </div>

      <div className="payment vcenter">
      <button className="buttonCreate center" onClick={removeAccount}><DeleteForeverIcon />  Remove Account</button>
      </div>
    </div>
  </div>
  )
}

export default Login;