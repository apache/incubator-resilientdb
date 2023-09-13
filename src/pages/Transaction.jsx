/*global chrome*/
import '../css/App.css';
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";
import { sendRequest } from '../client';
import React, { useRef, useState } from 'react';
import icon from "../images/logos/bitcoin.png";

function Transaction(props) {
    const location = useLocation();
    const [isCopied, setIsCopied] = useState(false);
    var content;

    if (location.state.from === 'commit') {
        content = ( 
        <form id="Form">
            <div className="form__row">
                <div className="form__select">
                    <p name="selectoptions" className="required">
                    Operation: COMMIT
                    </p>
                </div>
            </div>
            <div className="form__row d-flex align-items-center justify-space">
                <p className="form__input form__input--23">
                    {location.state.amount}
                </p>
                <div className="form__coin-icon"><img src={icon} alt="currency" /><span>ROK</span></div>
            </div>

            <div className="form__coin-total">ROK {location.state.amount}.00</div>
        </form>
    )
    } else if (location.state.from === 'get') {
        content = (
            <form id="Form">
            <div className="form__row">
                <div className="form__select">
                    <p name="selectoptions" className="required">
                    Operation: FETCH
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
        )
    } else if (location.state.from === 'update') {
        content = (
            <form id="Form">
            <div className="form__row">
                <div className="form__select">
                    <p name="selectoptions" className="required">
                    Operation: UPDATE
                    </p>
                </div>
            </div>
            <div className="form__row d-flex align-items-center justify-space">
                <p className="form__input form__input--23">
                    {location.state.amount}
                </p>
                <div className="form__coin-icon"><img src={icon} alt="currency" /><span>ROK</span></div>
            </div>

            <div className="form__coin-total">ROK {location.state.amount}.00</div>
            </form>
        )
    } else if (location.state.from === 'update-multi') {
        content = (
            <form id="Form">
            <div className="form__row">
                <div className="form__select">
                    <p name="selectoptions" className="required">
                    Operation: UPDATE ALL
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
        )
    } else if (location.state.from === 'filter') {
        content = (
            <form id="Form">
            <div className="form__row">
                <div className="form__select">
                    <p name="selectoptions" className="required">
                    Operation: FILTER
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
        )
    } else if (location.state.from === 'account') {
        content = (
            <form id="Form">
            <div className="form__row">
                <div className="form__select">
                    <p name="selectoptions" className="required">
                    Operation: INFO
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
        )
    }
    
    const submit = async () => {
        if ((location.state.from === 'commit')) {
        var escapeCodes = { 
            '\\': '\\',
            'r':  '\r',
            'n':  '\n',
            't':  '\t'
        };
        location.state.data.replace(/\\(.)/g, function(str, char) {
            return escapeCodes[char];
        });

        const query = `mutation {
            postTransaction(data: {
            operation: "CREATE",
            amount: ${parseInt(location.state.amount)},
            signerPublicKey: "${location.state.publicKey}",
            signerPrivateKey: "${location.state.privateKey}",
            recipientPublicKey: "${location.state.address}",
            asset: """{
                "data": { 
                    ${location.state.data}
                },
            }
            """
            }){
            id
            }
        }`

        const result = sendRequest(query).then(res => { 
            const store = location.state;
            store.id = res.data.postTransaction.id;
            chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
            // Send a message to the content script
            chrome.tabs.sendMessage(tabs[0].id, store.id)
            });
            store.history.push(store.id);
            chrome.storage.sync.set({ store }, () => {
                props.navigate("/logs", { state: store });
            }); 
        });
        }

        else if ((location.state.from === 'get')){
        const query = `query {
            getTransaction(id: "${location.state.id}"){
            id
            version
            amount
            metadata
            operation
            asset
            publicKey
            uri
            type
            }
        }`

        const result = sendRequest(query).then(res => { 
            const store = location.state;
            store.id = res.data.getTransaction.id;
            chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
            // Send a message to the content script
            chrome.tabs.sendMessage(tabs[0].id, res.data.getTransaction)
            });
            store.history.push(store.id);
            chrome.storage.sync.set({ store }, () => {
                props.navigate("/logs", { state: store });
            }); 
        });
        }

        else if ((location.state.from === 'update')) {
        escapeCodes = { 
            '\\': '\\',
            'r':  '\r',
            'n':  '\n',
            't':  '\t'
        };
        location.state.data.replace(/\\(.)/g, function(str, char) {
            return escapeCodes[char];
        });

        const query = `mutation {
            updateTransaction(data: {
            id: "${location.state.id}"
            operation: "",
            amount: ${parseInt(location.state.amount)},
            signerPublicKey: "${location.state.publicKey}",
            signerPrivateKey: "${location.state.privateKey}",
            recipientPublicKey: "${location.state.address}",
            asset: """{
                "data": { 
                    ${location.state.data}
                },
            }
            """
            }){
            id
            version
            amount
            metadata
            operation
            asset
            publicKey
            uri
            type
            }
        }`

        const result = sendRequest(query).then(res => { 
            const store = location.state;
            store.id = res.data.updateTransaction.id;
            chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
            // Send a message to the content script
            chrome.tabs.sendMessage(tabs[0].id, res.data.updateTransaction)
            });
            store.history.push(store.id);
            chrome.storage.sync.set({ store }, () => {
                props.navigate("/logs", { state: store });
            }); 
        });
        }

        else if ((location.state.from === 'update-multi')) {
        escapeCodes = { 
            '\\': '\\',
            'r':  '\r',
            'n':  '\n',
            't':  '\t'
        };
        var records = [];
        for (const value of location.state.values) {
            value.data.replace(/\\(.)/g, function(str, char) {
            return escapeCodes[char];
            });
            var object = {
            id: value.id,
            operation: "",
            amount: value.amount === "" ? "" : parseInt(value.amount),
            signerPublicKey: location.state.publicKey,
            signerPrivateKey: location.state.privateKey,
            recipientPublicKey: value.address,
            asset: `"""{ "data": { ${value.data} }, }"""`
            }
            records.push(object);
        }

        const query = `mutation {
            updateMultipleTransaction(data: [
            ${records.map(record => `{
                id: "${record.id}",
                operation: "${record.operation}",
                amount: ${record.amount}
                signerPublicKey: "${record.signerPublicKey}"
                signerPrivateKey: "${record.signerPrivateKey}"
                recipientPublicKey: "${record.recipientPublicKey}"
                asset: ${record.asset}
            }`).join(',')}
            ]){
            id
            version
            amount
            metadata
            operation
            asset
            publicKey
            uri
            type
            }
        }`

        const result = sendRequest(query).then(res => { 
            const store = location.state;
            store.id = res.data.updateMultipleTransaction[0].id;
            chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
            // Send a message to the content script
            chrome.tabs.sendMessage(tabs[0].id, res.data.updateMultipleTransaction)
            });
            store.history.push(store.id);
            chrome.storage.sync.set({ store }, () => {
                props.navigate("/logs", { state: store });
            }); 
        });
        }


        else if ((location.state.from === 'filter')) {
        const ownerPublicKey = location.state.ownerPublicKey !== null ? `"${location.state.ownerPublicKey}"` : null;
        const recipientPublicKey = location.state.recipientPublicKey !== null ? `"${location.state.recipientPublicKey}"` : null;
        const query = `query { getFilteredTransactions(filter: {
            ownerPublicKey: ${ownerPublicKey}
            recipientPublicKey: ${recipientPublicKey}
            }) {
            id
            version
            amount
            metadata
            operation
            asset
            publicKey
            uri
            type
            } 
        }`

        const result = sendRequest(query).then(res => { 
            const store = location.state;
            chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
            // Send a message to the content script
            chrome.tabs.sendMessage(tabs[0].id, res.data.getFilteredTransactions)
            });
            props.navigate("/logs", { state: store });
        });
        }

        else if ((location.state.from === 'account')) {
            const store = location.state;
            chrome.tabs.query({ active: true, currentWindow: true }, function(tabs) {
            // Send a message to the content script
            chrome.tabs.sendMessage(tabs[0].id, location.state.publicKey)
            });
            props.navigate("/logs", { state: store });
        }
    }

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
                        {content}
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

                <button className="button button--full button--main open-popup" onClick={submit}>Submit</button>
                <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>ResVault v0.0.1 Alpha Release</p>
            </div> 
        </div>
  )
}

export default Transaction;