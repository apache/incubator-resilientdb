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
import icon from "../images/logos/bitcoin.png";
import React from 'react';

function Logs(props) {
    const location = useLocation();
    const back = async () => {
        const store = location.state;
        chrome.storage.local.clear(function(){
          props.navigate("/login", {state: store});
        });
      }
    
      const dashboard = async () => {
        props.navigate("/dashboard", {state: location.state});
    }

    const store = location.state;
    const reversedData = [...store.history].reverse();
    const transactionsToDisplay = reversedData.slice(0, 5);
    const shortenTransactionId = (originalTxnId) => {
        return `${originalTxnId.slice(0, 7)}...${originalTxnId.slice(-7)}`;
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
                <h2 className="page__title">History</h2>
                
                <div className="page__title-bar">
				<h3>Recent Transactions</h3>
				<div className="page__title-right">
					<button className="button button--main button--ex-small" onClick={dashboard}>Back</button>
				</div>
			    </div>		
				
				  <div className="cards cards--11">
                  {transactionsToDisplay.map((transactionId, index) => (
                        <p className="card-coin" style={{ cursor: 'pointer' }} key={index}>
                        <div className="card-coin__logo">
                            <img src={icon} alt="currency" />
                            <span>
                            Transaction ID <b>{shortenTransactionId(transactionId)}</b>
                            </span>
                        </div>
                        </p>
                    ))}
				  </div>
				  
				 {/*<div className="w-100 text-center"><button style={{color: '#47e7ce', fontWeight: '600', fontSize: '1.2rem', background: 'transparent', border: 'none', 'outline': 'none', 'cursor': 'pointer'}}>View all transactions</button></div>*/}
                 <p className="bottom-navigation" style={{backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px'}}>ResVault v0.0.1 Alpha Release</p>
            </div> 
        </div>
  )
}

export default Logs;
