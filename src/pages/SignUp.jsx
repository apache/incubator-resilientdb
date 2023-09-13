/*global chrome*/
import '../css/App.css';
import CryptoJS from "crypto-js";
import { useAlert } from 'react-alert'
import IconButton from "@material-ui/core/IconButton";
import Visibility from "@material-ui/icons/Visibility";
import InputAdornment from "@material-ui/core/InputAdornment";
import VisibilityOff from "@material-ui/icons/VisibilityOff";
import Input from "@material-ui/core/Input";
import bcrypt from 'bcryptjs';
import Base58 from 'bs58';
import nacl from 'tweetnacl';
import { useLocation } from "react-router-dom";
import PasswordStrengthBar from 'react-password-strength-bar';
import backicon from "../images/icons/arrow-back.svg";

function SignUp(props) {
    const location = useLocation();
    const alert = useAlert();
    
    const back = async () => {
        props.navigate("/", {state: location.state});
    }

    const buttonStyle = {
        backgroundColor: 'transparent', // Remove background color
        cursor: 'pointer', // Change cursor to a pointer
        border: 'none', // Remove border
        boxShadow: 'none', // Remove shadow
        outline: 'none', // Remove focus outline
    };

    const salt = bcrypt.genSaltSync(10);

    function generateKeyPair(){
        var keyPair = nacl.sign.keyPair();
        var pk = Base58.encode(keyPair.publicKey);
        var sk = Base58.encode(keyPair.secretKey.slice(0,32));
        return {publicKey: pk, privateKey: sk};
    };

    const createAccount = async () => {
        if(props.values.password===props.confirmValues.password){
        chrome.storage.sync.clear(async function(){
            const keys = generateKeyPair();
            var publicKey = keys.publicKey;
            var privateKey = keys.privateKey;
            const phrase = CryptoJS.AES.encrypt(
            JSON.stringify(privateKey),
            props.values.password
            ).toString();
            var hash = bcrypt.hashSync(props.values.password, salt);
            const store = {publicKey: publicKey, encryptedPrivateKey: phrase, hash: hash};
            var password = {password: props.values.password};
            chrome.storage.local.set({ password }, () => {});
            store.history = [];
            chrome.storage.sync.set({ store }, () => {
            store.privateKey = privateKey;
            props.navigate("/dashboard", {state: store} );
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
    <div className="page page--login" data-page="login">
	<header className="header header--fixed">	
		<div className="header__inner">	
			<div className="header__icon">
                <button onClick={back} style={buttonStyle}><img src={backicon} alt="back" /></button>
            </div>	
        </div>
	</header>
	
        <div className="login">
		<div className="login__content">	
			<h2 className="login__title">Create an account</h2>
				<div className="login-form">
					<form id="LoginForm" method="post">
						<div className="login-form__row">
							<label className="login-form__label">Password</label>
							<Input
                            type={props.values.showPassword ? "text" : "password"}
                            onChange={handlePasswordChange("password")}
                            placeholder="Password"
                            className="login-form__input"
                            value={props.values.password}
                            style={{color: 'white', width: '100%'}}
                            disableUnderline
                            endAdornment={
                                <InputAdornment position="end">
                                <IconButton
                                    onClick={handleClickShowPassword}
                                    onMouseDown={handleMouseDownPassword}
                                    style={{color: 'white'}}
                                >
                                    {props.values.showPassword ? <Visibility /> : <VisibilityOff />}
                                </IconButton>
                                </InputAdornment>
                            }
                            />
						</div>
						<div className="login-form__row">
							<label className="login-form__label">Confirm Password</label>
							<Input
                            type={props.confirmValues.showPassword ? "text" : "password"}
                            onChange={handleConfirmPasswordChange("password")}
                            placeholder="Confirm Password"
                            className="login-form__input"
                            value={props.confirmValues.password}
                            style={{color: 'white', width: '100%'}}
                            disableUnderline
                            endAdornment={
                                <InputAdornment position="end">
                                <IconButton
                                    onClick={handleClickShowConfirmPassword}
                                    onMouseDown={handleMouseDownConfirmPassword}
                                    style={{color: 'white'}}
                                >
                                    {props.confirmValues.showPassword ? <Visibility /> : <VisibilityOff />}
                                </IconButton>
                                </InputAdornment>
                            }
                            />
						</div>
                        <PasswordStrengthBar password={props.values.password} />
						<div className="login-form__row">
							<button className="login-form__submit button button--main button--full" id="submit" onClick={createAccount}>
                                Create Account
                            </button>
						</div>
					</form>	
				</div>
                <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>ResVault v0.0.1 Alpha Release</p>
		</div>
        </div>
    </div>
  )
}

export default SignUp;