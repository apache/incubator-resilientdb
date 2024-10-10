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
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS
 * OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.    
 */

/*global chrome*/
import '../css/App.css';
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import AddCircleOutlineIcon from '@mui/icons-material/AddCircleOutline';
import ContentCopyIcon from '@mui/icons-material/ContentCopy';
import DownloadIcon from '@mui/icons-material/Download';
import React, { useRef, useState, useEffect, useContext } from 'react';
import Lottie from 'react-lottie';
import versionData from '../data/version.json';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faLock, faUnlock } from '@fortawesome/free-solid-svg-icons';
import graphql from "../images/logos/graphql.png";
import { GlobalContext } from '../context/GlobalContext';
import { useNavigate } from 'react-router-dom';

function Dashboard() {
    const {
        publicKey,
        privateKey,
        keyPairs,
        generateKeyPair,
        setPublicKey,
        setPrivateKey,
        selectedKeyPairIndex,
        setSelectedKeyPairIndex,
        setSelectedKeyPair,
        setIsAuthenticated,
    } = useContext(GlobalContext);

    const [tabId, setTabId] = useState(null);
    const [domain, setDomain] = useState('');
    const [selectedNet, setSelectedNet] = useState('');
    const [completeUrl, setCompleteUrl] = useState('');
    const [customUrl, setCustomUrl] = useState('');
    const [isCopied, setIsCopied] = useState(false);
    const [isConnected, setIsConnected] = useState(false);
    const [faviconUrl, setFaviconUrl] = useState('');
    const [nets, setNets] = useState([]);
    const [showModal, setShowModal] = useState(false);
    const [newNetName, setNewNetName] = useState('');
    const [protocol, setProtocol] = useState('http');
    const [error, setError] = useState('');
    const [jsonFileName, setJsonFileName] = useState('');
    const fileInputRef = useRef(null);
    const navigate = useNavigate();

    // New state variables for transaction data and handling
    const [transactionData, setTransactionData] = useState(null);
    const [transactionError, setTransactionError] = useState('');
    const [showSuccessModal, setShowSuccessModal] = useState(false);
    const [successResponse, setSuccessResponse] = useState(null);

    // New state for copying transaction ID
    const [isIdCopied, setIsIdCopied] = useState(false);

    const defaultOptions = {
        loop: true,
        autoplay: true,
        animationData: require('../images/bg.json'),
        rendererSettings: {
        preserveAspectRatio: 'xMidYMid slice',
        renderer: 'svg'
        }
    };

    const modalOptions = {
        loop: true,
        autoplay: true,
        animationData: require('../images/modal.json'),
        rendererSettings: {
        preserveAspectRatio: 'xMidYMid slice',
        renderer: 'svg'
        }
    };

    // Function to get full hostname from URL
    function getBaseDomain(url) {
        try {
        const urlObj = new URL(url);
        return urlObj.hostname;
        } catch (error) {
        console.error('Invalid URL:', url);
        return '';
        }
    }

    // Function to set connection status per domain and net
    const setDomainConnectionStatus = (domain, net, isConnected) => {
        chrome.storage.local.get(['connections'], (result) => {
        let connections = result.connections || {};
        if (!connections[domain]) {
            connections[domain] = {};
        }
        connections[domain][net] = isConnected;
        chrome.storage.local.set({ connections });
        });
    };

    // Function to get connection status per domain and net
    const getDomainConnectionStatus = (domain, net, callback) => {
        chrome.storage.local.get(['connections'], (result) => {
        const connections = result.connections || {};
        const domainConnections = connections[domain] || {};
        const isConnected = domainConnections[net] || false;
        callback(isConnected);
        });
    };

    useEffect(() => {
        chrome.tabs.query({ active: true, currentWindow: true }, (tabs) => {
        if (tabs.length > 0 && tabs[0].url) {
            const currentTab = tabs[0];
            setTabId(currentTab.id);

            const currentDomain = getBaseDomain(currentTab.url);
            setDomain(currentDomain);

            // Ensure selectedNet is set before checking connection status
            if (selectedNet) {
            getDomainConnectionStatus(currentDomain, selectedNet, (connected) => {
                setIsConnected(connected);
            });
            }
        } else {
            // If no active tab or no URL, set domain to 'extension'
            setDomain('extension');

            // Ensure selectedNet is set before checking connection status
            if (selectedNet) {
            getDomainConnectionStatus('extension', selectedNet, (connected) => {
                setIsConnected(connected);
            });
            }
        }
        });
    }, [selectedNet]);

    useEffect(() => {
        if (domain && selectedNet) {
        chrome.runtime.sendMessage({ action: "requestData", domain: domain, net: selectedNet }, function(response) {
            if (response && response.faviconUrl) {
            setFaviconUrl(response.faviconUrl);
            } else {
            setFaviconUrl('');
            }
            // Handle other response data if needed
        });
        }
    }, [domain, selectedNet]);

    useEffect(() => {
        const storedNets = JSON.parse(localStorage.getItem('nets')) || [];
        setNets(storedNets);
    }, []);

    useEffect(() => {
        // Fetch active net URL from storage
        chrome.storage.local.get(['activeNetUrl'], (result) => {
        if (result.activeNetUrl) {
            setCompleteUrl(result.activeNetUrl); // Use the full URL with protocol

            // Check if it's one of the known networks
            if (result.activeNetUrl === 'https://cloud.resilientdb.com/graphql') {
            setSelectedNet('ResilientDB Mainnet');
            } else if (result.activeNetUrl === 'http://localhost:8000/graphql') {
            setSelectedNet('ResilientDB Localnet');
            } else {
            // Custom URL case
            const customNet = nets.find(net => net.url === result.activeNetUrl);
            if (customNet) {
                setSelectedNet(customNet.name); // Set the name of the custom network
            } else {
                setSelectedNet('Custom URL');
            }
            // Set customUrl by stripping the protocol and /graphql part
            setCustomUrl(result.activeNetUrl.replace(/^https?:\/\/|\/graphql$/g, ''));
            }
        } else {
            // No active net URL, default to ResilientDB Mainnet
            setCompleteUrl('https://cloud.resilientdb.com/graphql');
            setSelectedNet('ResilientDB Mainnet'); // Ensure default network is selected
        }
        });
    }, [nets]);

    useEffect(() => {
        if (domain && selectedNet) {
        getDomainConnectionStatus(domain, selectedNet, (connected) => {
            setIsConnected(connected);
        });
        }
    }, [domain, selectedNet]);

    const toggleProtocol = () => {
        const newProtocol = protocol === 'http' ? 'https' : 'http';
        setProtocol(newProtocol);
        if (selectedNet === 'Custom URL' && customUrl) {
        setCompleteUrl(`${newProtocol}://${customUrl}/graphql`);
        }
    };

    const addNet = () => {
        if (!newNetName.trim() || !customUrl.trim()) {
        setError('Both fields are required.');
        return;
        }
        setError('');
        const fullUrl = `${protocol}://${customUrl}/graphql`;
        const newNets = [...nets, { name: newNetName, url: fullUrl }];
        setNewNetName('');
        setCustomUrl('');
        setNets(newNets);
        localStorage.setItem('nets', JSON.stringify(newNets));
        setSelectedNet(newNetName);
    };

    const removeNet = (name) => {
        const filteredNets = nets.filter(net => net.name !== name);
        setNets(filteredNets);
        localStorage.setItem('nets', JSON.stringify(filteredNets));
    };

    const toggleConnection = () => {
        if (!publicKey || !privateKey) {
        console.error('Public or Private key is missing');
        return;
        }

        const newConnectionStatus = !isConnected;
        setIsConnected(newConnectionStatus);

        if (newConnectionStatus) {
        console.log('Connecting to net:', selectedNet, 'on domain:', domain);
        chrome.runtime.sendMessage(
            {
            action: 'storeKeys',
            publicKey,
            privateKey,
            url: completeUrl,
            domain: domain,
            net: selectedNet,
            },
            () => {
            setDomainConnectionStatus(domain, selectedNet, newConnectionStatus);
            }
        );
        } else {
        console.log('Disconnecting from net:', selectedNet, 'on domain:', domain);
        chrome.runtime.sendMessage(
            {
            action: 'disconnectKeys',
            domain: domain,
            net: selectedNet,
            },
            () => {
            setDomainConnectionStatus(domain, selectedNet, false);
            }
        );
        }
    };

    const switchNetwork = (value) => {
        if (value === 'Manage Nets') {
        setShowModal(true);
        } else {
        let newCompleteUrl = '';
        switch (value) {
            case 'ResilientDB Mainnet':
            newCompleteUrl = 'https://cloud.resilientdb.com/graphql';
            break;
            case 'ResilientDB Localnet':
            newCompleteUrl = 'http://localhost:8000/graphql';
            break;
            case 'Custom URL':
            if (customUrl) {
                newCompleteUrl = `${protocol}://${customUrl}/graphql`;
            } else {
                newCompleteUrl = `${protocol}://`;
            }
            break;
            default:
            const selectedNetwork = nets.find(net => net.name === value);
            if (selectedNetwork) {
                newCompleteUrl = selectedNetwork.url;
                setCustomUrl(selectedNetwork.url.split('://')[1].replace('/graphql', ''));
            }
            break;
        }
        setCompleteUrl(newCompleteUrl);
        setSelectedNet(value); // Set this at the end to avoid premature updates

        // Update activeNetUrl in storage using the new URL
        chrome.storage.local.set({ activeNetUrl: newCompleteUrl }, () => {
            console.log('Active net URL updated to', newCompleteUrl);
        });
        }
    };

    const handleNetworkChange = (event) => {
        const value = event.target.value;
        if (value === selectedNet) return; // Ignore if the value hasn't changed

        if (isConnected) {
        // Disconnect from the current network and clear domain connection status
        chrome.runtime.sendMessage({
            action: 'disconnectKeys',
            domain: domain,
            net: selectedNet
        }, () => {
            console.log('Disconnected from previous network');
            setIsConnected(false);
            setDomainConnectionStatus(domain, selectedNet, false);
            // Proceed with switching networks
            switchNetwork(value);
        });
        } else {
        // If not connected, just switch networks
        switchNetwork(value);
        }
    };

    const handleCustomUrlChange = (event) => {
        const value = event.target.value;
        setCustomUrl(value);
        const newUrl = `${protocol}://${value}/graphql`;
        setCompleteUrl(newUrl);
    };

    const back = () => {
        chrome.storage.local.remove(['password'], function() {
          setIsAuthenticated(false);
          navigate("/login");
        });
    };

    // Function to copy public key
    const handleCopyPublicKey = () => {
        try {
        const tempInput = document.createElement('input');
        tempInput.value = publicKey;
        document.body.appendChild(tempInput);
        tempInput.select();
        document.execCommand('copy');
        document.body.removeChild(tempInput);
        setIsCopied(true);
        setTimeout(() => {
            setIsCopied(false);
        }, 1500);
        } catch (err) {
        console.error('Unable to copy text: ', err);
        }
    };

    // Function to download key pair as JSON
    const handleDownloadKeyPair = () => {
        const keyPair = {
        publicKey: publicKey,
        privateKey: privateKey,
        };
        const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(keyPair));
        const downloadAnchorNode = document.createElement('a');
        downloadAnchorNode.setAttribute("href", dataStr);
        downloadAnchorNode.setAttribute("download", "keypair.json");
        document.body.appendChild(downloadAnchorNode); // required for firefox
        downloadAnchorNode.click();
        downloadAnchorNode.remove();
    };

    const handleFileUpload = (e) => {
        const file = e.target.files[0];
        if (file && file.type === 'application/json') {
        setJsonFileName(file.name); // Show file name once uploaded

        const reader = new FileReader();
        reader.onload = (event) => {
            try {
            const json = JSON.parse(event.target.result);
            // Validate JSON data
            if (json.asset && json.recipientAddress && json.amount) {
                setTransactionData(json);
                setTransactionError(''); // Clear any previous error
            } else {
                setTransactionData(null);
                setTransactionError('Invalid JSON format: Missing required fields.');
            }
            } catch (err) {
            console.error('Error parsing JSON:', err);
            setTransactionData(null);
            setTransactionError('Invalid JSON format.');
            }
        };
        reader.readAsText(file);
        } else {
        setJsonFileName(''); // Clear if the file is not JSON
        setTransactionData(null);
        setTransactionError('Please upload a JSON file.');
        }
    };

    const handleDragEnter = (e) => {
        e.preventDefault();
        e.stopPropagation();
    };

    const handleDragOver = (e) => {
        e.preventDefault();
        e.stopPropagation();
    };

    const handleDrop = (e) => {
        e.preventDefault();
        e.stopPropagation();
        const file = e.dataTransfer.files[0];
        if (file && file.type === 'application/json') {
        setJsonFileName(file.name);

        const reader = new FileReader();
        reader.onload = (event) => {
            try {
            const json = JSON.parse(event.target.result);
            // Validate JSON data
            if (json.asset && json.recipientAddress && json.amount) {
                setTransactionData(json);
                setTransactionError(''); // Clear any previous error
            } else {
                setTransactionData(null);
                setTransactionError('Invalid JSON format: Missing required fields.');
            }
            } catch (err) {
            console.error('Error parsing JSON:', err);
            setTransactionData(null);
            setTransactionError('Invalid JSON format.');
            }
        };
        reader.readAsText(file);
        } else {
        setJsonFileName('');
        setTransactionData(null);
        setTransactionError('Please upload a JSON file.');
        }
    };

    const handleFileClick = () => {
        fileInputRef.current.click(); // Open file explorer when clicking on the field
    };

    const handleSubmit = () => {
        if (!transactionData) {
        setTransactionError('No valid transaction data found.');
        return;
        }
        if (!isConnected) {
        setTransactionError('Please connect to a net before submitting a transaction.');
        return;
        }

        // Send transaction data to background script
        chrome.runtime.sendMessage({
        action: 'submitTransactionFromDashboard',
        transactionData: transactionData,
        domain: domain,
        net: selectedNet,
        }, (response) => {
        if (response.success) {
            setSuccessResponse(response.data);
            setShowSuccessModal(true);
            setTransactionError('');
            setJsonFileName(''); // Clear the file name after successful submission
            setTransactionData(null);
        } else {
            setTransactionError(response.error || 'Transaction submission failed.');
        }
        });
    };

    // New function to handle transaction ID click
    const handleIdClick = () => {
        try {
        const transactionId = (successResponse && successResponse.postTransaction && successResponse.postTransaction.id) || '';
        const tempInput = document.createElement('input');
        tempInput.value = transactionId;
        document.body.appendChild(tempInput);
        tempInput.select();
        document.execCommand('copy');
        document.body.removeChild(tempInput);
        setIsIdCopied(true);
        setTimeout(() => {
            setIsIdCopied(false);
        }, 1500);
        } catch (err) {
        console.error('Unable to copy text: ', err);
        }
    };

    // **New function to handle favicon load error**
    const handleFaviconError = () => {
        setFaviconUrl(''); // This will trigger the globe icon to display
    };

    const disconnectDueToKeysChange = () => {
        if (isConnected) {
            // Disconnect from the current network and clear domain connection status
            chrome.runtime.sendMessage({
                action: 'disconnectKeys',
                domain: domain,
                net: selectedNet
            }, () => {
                console.log('Disconnected from previous network');
                setIsConnected(false);
                setDomainConnectionStatus(domain, selectedNet, false);
            });
        }
    }

    const switchKeyPair = (index) => {
        setSelectedKeyPair(index);
        disconnectDueToKeysChange();
    };

    const handleGenerateKeyPair = () => {
        generateKeyPair(() => {
          setSelectedKeyPairIndex(keyPairs.length); // Select the newly generated key pair
          disconnectDueToKeysChange();
        });
    };
      

    return (
        <>
        <div className="lottie-background">
            <Lottie options={defaultOptions} height="100%" width="100%" />
        </div>
        <div className="page page--main" data-page="buy">
            <header className="header header--fixed">
            <div className="header__inner header-container">
                <div className="header__logo header__logo--text">
                Res<strong>Vault</strong>
                </div>
                <div className="badge-container">
                <span className="badge">KV Service</span>
                </div>
                <div className="header__icon open-panel">
                <button
                    style={{ background: 'none', color: 'white', fontWeight: 'bolder', outline: 'none', borderStyle: 'none', cursor: 'pointer' }}
                    onClick={back}
                >
                    <ExitToAppIcon />
                </button>
                </div>
            </div>
            </header>

            {showModal && (
            <div className="overlay">
                <div className="modal">
                <div className="lottie-modal-background">
                    <Lottie options={modalOptions} height="100%" width="100%" />
                </div>
                <div className="modal-content">
                    <h2>Manage Nets</h2>
                    {nets.length > 0 && (
                    <div className="table-container">
                        <table>
                        <thead>
                            <tr>
                            <th>Net Name</th>
                            <th>Action</th>
                            </tr>
                        </thead>
                        <tbody>
                            {nets.map(net => (
                            <tr key={net.name}>
                                <td>{net.name}</td>
                                <td>
                                <button className="icon-button" onClick={() => removeNet(net.name)}>
                                    <i className="fas fa-trash"></i>
                                </button>
                                </td>
                            </tr>
                            ))}
                        </tbody>
                        </table>
                    </div>
                    )}
                    <input className="modal-net-input" type="text" placeholder="Net Name" value={newNetName} onChange={e => setNewNetName(e.target.value)} />
                    <div className="modal-url-input-container">
                    <div className="modal-url-toggle" onClick={toggleProtocol}>
                        <FontAwesomeIcon icon={protocol === 'https' ? faLock : faUnlock} className={`icon ${protocol === 'https' ? 'icon-green' : 'icon-red'}`} />
                    </div>
                    <input type="text" placeholder="GraphQL URL" value={customUrl} onChange={handleCustomUrlChange} className="modal-url-input" />
                    <div className="modal-url-fixed">
                        <img src={graphql} className="graphql-icon" alt="GraphQL"></img>
                    </div>
                    </div>
                    <div className="save-container">
                    <button onClick={addNet} className="button-save">
                        Save
                    </button>
                    <button onClick={() => setShowModal(false)} className="button-close">
                        Close
                    </button>
                    </div>
                    {error && <p className="error-message">{error}</p>}
                </div>
                </div>
            </div>
            )}

            {showSuccessModal && (
            <div className="overlay">
                <div className="modal">
                <div className="modal-content">
                    <h2>Transaction Submitted Successfully!</h2>
                    {/* Extract transaction ID */}
                    {successResponse && successResponse.postTransaction && successResponse.postTransaction.id ? (
                    <div className="fieldset">
                    <div className="radio-option radio-option--full">
                        <input
                        type="radio"
                        name="transactionId"
                        id="txId"
                        value={successResponse.postTransaction.id}
                        checked
                        readOnly
                        onClick={handleIdClick}
                        />
                        <label htmlFor="txId">
                        <span>{isIdCopied ? 'Copied' : `${successResponse.postTransaction.id.slice(0, 5)}...${successResponse.postTransaction.id.slice(-5)}`}</span>
                        </label>
                    </div>
                    </div>
                    ) : (
                    <p>No transaction ID found.</p>
                    )}
                    <button onClick={() => setShowSuccessModal(false)} className="button-close">
                    Close
                    </button>
                </div>
                </div>
            </div>
            )}

            <div className="page__content page__content--with-header page__content--with-bottom-nav">
            <h2 className="page__title">Dashboard</h2>

            <div className="net">
                <div className="net-header">
                <div className="select-wrapper">
                    <select value={selectedNet} onChange={handleNetworkChange} className="select">
                    <option value="ResilientDB Mainnet">ResilientDB Mainnet</option>
                    <option value="ResilientDB Localnet">ResilientDB Localnet</option>
                    {nets.map(net => (
                        <option key={net.name} value={net.name}>{net.name}</option>
                    ))}
                    <option value="Manage Nets">Manage Nets</option>
                    </select>
                    <i className="fas fa-chevron-down"></i>
                </div>

                <div className="icon-container" onClick={toggleConnection}>
                    {faviconUrl ? (
                    <img
                        src={faviconUrl}
                        alt="Favicon"
                        className={`icon ${isConnected ? 'connected' : ''}`}
                        onError={handleFaviconError}
                    />
                    ) : (
                    <i className={`fa fa-globe icon ${isConnected ? 'connected' : ''}`} aria-hidden="true"></i>
                    )}
                    <span className="status-dot"></span>
                    <span className="tooltip">{isConnected ? 'Connected' : 'Disconnected'}</span>
                </div>
                {selectedNet === 'Custom Net' && (
                    <input
                    type="text"
                    value={customUrl}
                    onChange={handleCustomUrlChange}
                    placeholder="Enter custom URL"
                    className="input"
                    />
                )}
                </div>

                <div className="file-upload">
                    <div
                    className={`drag_box_outline ${jsonFileName ? 'file-uploaded' : ''}`}
                    onDragEnter={handleDragEnter}
                    onDragOver={handleDragOver}
                    onDrop={handleDrop}
                    onClick={handleFileClick}
                    >
                    <input
                        type="file"
                        ref={fileInputRef}
                        style={{ display: 'none' }}
                        accept="application/json"
                        onChange={handleFileUpload}
                    />
                    {jsonFileName ? (
                        <span className="filename">{jsonFileName} uploaded</span>
                    ) : (
                        <span className="filename">Click to Upload JSON File</span>
                    )}
                    </div>
                    {transactionError && <p className="error-message">{transactionError}</p>}
                </div>
            </div>

            

            <h2 className="page__title">Select Account</h2>
            <div className="net">
                <div className="keypair">
                    <div className="keypair-header">
                    <div className="select-wrapper">
                        <select
                        value={selectedKeyPairIndex}
                        onChange={(e) => switchKeyPair(e.target.value)}
                        className="select"
                        >
                        {keyPairs.map((keyPair, index) => (
                            <option key={index} value={index}>
                            {`${keyPair.publicKey.slice(0, 4)}...${keyPair.publicKey.slice(-4)}`}
                            </option>
                        ))}
                        </select>
                        <i className="fas fa-chevron-down"></i>
                    </div>
                    <div className="keypair-icons">
                        <button onClick={handleGenerateKeyPair} className="icon-button">
                            <AddCircleOutlineIcon style={{ color: 'white' }} />
                        </button>
                        <button onClick={handleCopyPublicKey} className="icon-button">
                            <ContentCopyIcon style={{ color: isCopied ? 'green' : 'white' }} />
                        </button>
                        <button onClick={handleDownloadKeyPair} className="icon-button">
                            <DownloadIcon style={{ color: 'white' }} />
                        </button>
                    </div>
                    </div>
                </div>
            </div>

            <button className="button button--full button--main open-popup" onClick={handleSubmit}>
                Submit
            </button>
            <p className="bottom-navigation" style={{ backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px' }}>
                ResVault v{versionData.version}
            </p>
            </div>
        </div>
        </>
    );
}

export default Dashboard;