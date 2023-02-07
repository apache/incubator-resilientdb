/*global chrome*/
import logo from '../logo.png';
import '../App.css';
import CryptoJS from "crypto-js";
import Footer from "../components/Footer";
import { useAlert } from 'react-alert'
import IconButton from "@material-ui/core/IconButton";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";
import DeleteForeverIcon from '@mui/icons-material/DeleteForever';
import bcrypt from 'bcryptjs';

function Login(props) {
  const alert = useAlert();
  props.setFooter("footerLogin");

  const removeAccount = async () => {
    chrome.storage.sync.clear(function(){
      /*Cleared storage*/
    });
    props.navigate("/");
  }

  const loginAccount = async () => {
    if(bcrypt.compareSync(props.values.password, props.hash)){
      console.log(props.encryptedPrivateKey);
      console.log(props.values.password);
      try {
        const bytes = CryptoJS.AES.decrypt(props.encryptedPrivateKey, props.values.password);
        const data = JSON.parse(bytes.toString(CryptoJS.enc.Utf8));
        props.setPrivateKey(data);
        props.navigate("/dashboard");
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
    props.setValues({ ...props.values, showPassword: !props.values.showPassword });
  };
  
  const handleMouseDownPassword = (event) => {
    event.preventDefault();
  };
  
  const handlePasswordChange = (prop) => (event) => {
    props.setValues({ ...props.values, [prop]: event.target.value });
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
        <button className="buttonCreate center" onClick={loginAccount}>Login</button>
      </div>

      <div className="payment vcenter">
      <button className="buttonCreate center" onClick={removeAccount}><DeleteForeverIcon />  Remove Account</button>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Login;