/**
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
*/


/*global chrome*/
import '../css/App.css';
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";
import React, { useRef, useState } from 'react';
import { OPS, OP_DATA } from '../constants';


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

    const [isJSONInvalid, setIsJSONInvalid] = useState(false);

    const showError = () => {
        setIsJSONInvalid(true);
    }
    const handleDragEnter = (e) => {
        e.preventDefault();
        e.stopPropagation();
        // reset error state on drag
        setIsJSONInvalid(false);
    };
    const handleDragOver = (e) => {
        e.preventDefault();
        e.stopPropagation();
    };
    const handleDrop = (e) => {
        e.preventDefault();
        e.stopPropagation();

        // only text drops allowed
        if (e.dataTransfer.types.includes("text/plain")) {
            const droppedText = e.dataTransfer.getData("text/plain");
            // checking for a valid json format
            if (/^[\],:{}\s]*$/.test(droppedText.replace(/\\["\\\/bfnrtu]/g, '@').
                replace(/"[^"\\\n\r]*"|true|false|null|-?\d+(?:\.\d*)?(?:[eE][+\-]?\d+)?/g, ']').
                replace(/(?:^|:|,)(?:\s*\[)+/g, ''))) {
                const parsedJSON = JSON.parse(droppedText.replace(/,\s*([\]}])/g, '$1'));
                if ('operation' in parsedJSON && parsedJSON.operation in OPS) {
                    const OP = parsedJSON.operation;
                    const {from, requiredFields} = OP_DATA[OPS[OP]];
                    // check if all operation specific fields are present in json
                    if (requiredFields.every((field) => field in parsedJSON)) {
                        const store = location.state;
                        store.operation = OPS[OP];
                        store.from = from;
                        requiredFields.forEach((field) => {
                            store[field] = parsedJSON[field];
                        });
                        props.navigate("/transaction", {state: store});
                    } else {
                        showError();  
                    }
                } else {
                    showError();
                }
            } else {
                showError();
            }
        } else {
            showError();
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
                <div
                  className="fieldset"
                  onDrop={(e) => handleDrop(e)}
                  onDragEnter={(e) => handleDragEnter(e)}
                  onDragOver={(e) => handleDragOver(e)}
                >
                    <div className="drag_box">
                        <div className={`drag_box_outline${isJSONInvalid ? " erred" : ""}`}>
                            <span>{!isJSONInvalid ? <>Drag and Drop <b>JSON</b> here</> : "Invalid JSON"}</span>
                        </div>
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
