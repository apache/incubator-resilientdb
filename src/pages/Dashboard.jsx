/*global chrome*/
import '../css/App.css';
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";
import icon from "../images/logos/bitcoin.png";
import React, { useRef, useState } from 'react';


function Dashboard(props) {
    const location = useLocation();
    const [isCopied, setIsCopied] = useState(false);

    chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
        if ((msg.from === 'commit')) {
          const store = location.state;
          store.address = msg.address;
          store.amount = msg.amount;
          store.data = msg.data;
          store.from = msg.from;
          store.operation = "CREATE";
          props.navigate("/transaction", {state: store} );
        }
    
        else if ((msg.from === 'get')){
          const store = location.state;
          store.id = msg.id;
          store.from = msg.from;
          store.operation = "FETCH";
          props.navigate("/transaction", {state: store} );
        }
    
        else if ((msg.from === 'update')){
          const store = location.state;
          store.id = msg.id;
          store.address = msg.address;
          store.amount = msg.amount;
          store.data = msg.data;
          store.from = msg.from;
          store.operation = "UPDATE";
          props.navigate("/transaction", {state: store} );
        }
    
        else if ((msg.from === 'update-multi')){
          const store = location.state;
          const receivedValuesList = msg.values;
          store.values = receivedValuesList;
          store.from = msg.from;
          store.operation = "UPDATE MULTIPLE";
          props.navigate("/transaction", {state: store} );
        }
    
        else if ((msg.from === 'filter')){
          const store = location.state;
          store.ownerPublicKey = msg.ownerPublicKey;
          store.recipientPublicKey = msg.recipientPublicKey;
          store.from = msg.from;
          store.operation = "FILTER";
          props.navigate("/transaction", {state: store} );
        }
    
        else if ((msg.from === 'account')){
          const store = location.state;
          store.from = msg.from;
          store.operation = "ACCOUNT";
          props.navigate("/transaction", {state: store} );
        };
      });
    
      const back = async () => {
        const store = location.state;
        chrome.storage.local.clear(function(){
          props.navigate("/login", {state: store});
        });
      }

    const originalAccountId = location.state.publicKey;
    const shortenedId = `${originalAccountId.slice(0, 5)}...${originalAccountId.slice(-5)}`;

    const inputRef = useRef(null);

    const handleInputClick = () => {
        try {
            const tempInput = document.createElement('input');
            tempInput.value = originalAccountId;
            document.body.appendChild(tempInput);
            tempInput.select();
            document.execCommand('copy');
            document.body.removeChild(tempInput);
            setIsCopied(true);
                setTimeout(() => {
            setIsCopied(false);
        }, 1500); // Hide the notification after 1.5 seconds
        } catch (err) {
            console.error('Unable to copy text: ', err);
        }
    };

    return (
        <div className="page page--main" data-page="buy"> 
            <header className="header header--fixed">	
                <div className="header__inner">	
                    <div className="header__logo header__logo--text" style={{cursor: "pointer"}}>Res<strong>Vault</strong></div>	
                    <div className="header__icon open-panel" data-panel="left"><button style={{background: 'none', color: 'white', fontWeight: 'bolder', outline: 'none', borderStyle: 'none', cursor: 'pointer'}} onClick={back}><ExitToAppIcon /></button></div>
                </div>
            </header>
            
            <div className="page__content page__content--with-header page__content--with-bottom-nav">
                <h2 className="page__title">Dashboard</h2>
                <div className="fieldset">
                    <div className="form">
                        <form id="Form">
                            <div className="form__row">
                                <div className="form__select">
                                    <p name="selectoptions" className="required">
                                    Operation:
                                    </p>
                                </div>
                            </div>
                            <div className="form__row d-flex align-items-center justify-space">
                                <p className="form__input form__input--23">
                                    0
                                </p>
                                <div className="form__coin-icon"><img src={icon} alt="currency" /><span>ROK</span></div>
                            </div>

                            <div className="form__coin-total">ROK 0.00</div>
                        </form>
                    </div>
                </div>
                <h2 className="page__title">Select Account</h2>
                <div className="fieldset">
                    <div className="radio-option radio-option--full">
                    <input
                        type="radio"
                        name="wallet"
                        id="w3"
                        value={originalAccountId}
                        checked
                        readOnly
                        onClick={handleInputClick}
                        ref={inputRef}
                    />
                    <label htmlFor="w3">
                        <span>{isCopied ? 'Copied' : shortenedId}</span>
                    </label>
                    </div>
                </div>

                <button className="button button--full button--main open-popup">Submit</button>
                <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>ResVault v0.0.1 Alpha Release</p>
            </div> 
        </div>
  )
}

export default Dashboard;