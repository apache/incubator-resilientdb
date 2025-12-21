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
import DeleteIcon from '@mui/icons-material/Delete';
import SaveAltIcon from '@mui/icons-material/SaveAlt';
import UploadIcon from '@mui/icons-material/Upload';
import React, { useRef, useState, useEffect, useContext } from 'react';
import Lottie from 'react-lottie';
import versionData from '../data/version.json';
import { FontAwesomeIcon } from '@fortawesome/react-fontawesome';
import { faLock, faUnlock } from '@fortawesome/free-solid-svg-icons';
import graphql from "../images/logos/graphql.png";
import { GlobalContext } from '../context/GlobalContext';
import { useNavigate } from 'react-router-dom';
import { keccak256 } from 'js-sha3';
import Base58 from 'bs58';

function Contract() {
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
        deleteKeyPair,
        appendKeyPairs, 
        alert, // For alert modal
        setAlert, // For alert modal
        serviceMode,
        toggleServiceMode,
        ownerAddress,
        setOwnerAddress,
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
    const keyPairFileInputRef = useRef(null);
    const navigate = useNavigate();

    // New State Variables for Deployment
    const [solidityFileName, setSolidityFileName] = useState('');
    const [solidityContent, setSolidityContent] = useState('');
    const [deployJsonFileName, setDeployJsonFileName] = useState('');
    const [deployJsonContent, setDeployJsonContent] = useState('');
    const [deployError, setDeployError] = useState('');
    const [showContractModal, setShowContractModal] = useState(false);
    const [contractAddress, setContractAddress] = useState('');
    const [copyMessage, setCopyMessage] = useState('');

    // Add the missing state declaration
    const [showDeleteModal, setShowDeleteModal] = useState(false);

    // State for copying Contract Address
    const [isAddressCopied, setIsAddressCopied] = useState(false);

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

    // Function to generate owner's address
    const generateOwnerAddress = (publicKey) => {
        const decodedPublicKey = Base58.decode(publicKey);
        const addressHash = keccak256(Buffer.from(decodedPublicKey));
        const address = `0x${addressHash.slice(-40)}`;
        return address;
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

    // Update useEffect to set default URLs based on serviceMode
    useEffect(() => {
        // Fetch active net URL from storage
        chrome.storage.local.get(['activeNetUrl'], (result) => {
        if (result.activeNetUrl) {
            setCompleteUrl(result.activeNetUrl); // Use the full URL with protocol

            // Check if it's one of the known networks
            if (result.activeNetUrl === 'https://cloud.resilientdb.com/graphql' || result.activeNetUrl === 'https://contract.resilientdb.com/graphql') {
            setSelectedNet('ResilientDB Mainnet');
            } else if (result.activeNetUrl === 'http://localhost:8000/graphql' || result.activeNetUrl === 'http://localhost:8400/graphql') {
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
            const defaultUrl = serviceMode === 'KV' ? 'https://cloud.resilientdb.com/graphql' : 'https://contract.resilientdb.com/graphql';
            setCompleteUrl(defaultUrl);
            setSelectedNet('ResilientDB Mainnet'); // Ensure default network is selected
        }
        });
    }, [nets, serviceMode]);

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
        setAlert({ isOpen: true, message: 'Both Net Name and GraphQL URL are required.' });
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
        setAlert({ isOpen: true, message: 'Public or Private key is missing.' });
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

    // Update the switchNetwork function to use URLs based on serviceMode
    const switchNetwork = (value) => {
        if (value === 'Manage Nets') {
        setShowModal(true);
        } else {
        let newCompleteUrl = '';
        switch (value) {
            case 'ResilientDB Mainnet':
            newCompleteUrl = serviceMode === 'KV' ? 'https://cloud.resilientdb.com/graphql' : 'https://contract.resilientdb.com/graphql';
            break;
            case 'ResilientDB Localnet':
            newCompleteUrl = serviceMode === 'KV' ? 'http://localhost:8000/graphql' : 'http://localhost:8400/graphql';
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

    const handleDeleteKeyPair = () => {
        if (keyPairs.length > 1) {
            deleteKeyPair(selectedKeyPairIndex, () => {
                // Reset to the first key pair after deletion
                setSelectedKeyPairIndex(0);
                setSelectedKeyPair(0);
            });

            // Close the delete confirmation modal
            setShowDeleteModal(false);
        }
    };       

    // Function to copy owner address
    const handleCopyOwnerAddress = () => {
        try {
        const tempInput = document.createElement('input');
        tempInput.value = ownerAddress;
        document.body.appendChild(tempInput);
        tempInput.select();
        document.execCommand('copy');
        document.body.removeChild(tempInput);
        setIsCopied(true);
        setTimeout(() => {
            setIsCopied(false);
        }, 1500);
        } catch (err) {
        setAlert({ isOpen: true, message: 'Unable to copy text.' });
        console.error('Unable to copy text: ', err);
        }
    };

    // Function to download key pair including owner address
    const handleDownloadKeyPair = () => {
        const keyPair = {
        ownerAddress: ownerAddress,
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

    // Function to download all key pairs including owner addresses
    const handleDownloadAllKeyPairs = () => {
        const allKeyPairs = keyPairs.map(({ publicKey, privateKey }) => {
        const ownerAddress = generateOwnerAddress(publicKey);
        return {
            ownerAddress,
            publicKey,
            privateKey,
        };
        });
        const dataStr = "data:text/json;charset=utf-8," + encodeURIComponent(JSON.stringify(allKeyPairs));
        const downloadAnchorNode = document.createElement('a');
        downloadAnchorNode.setAttribute("href", dataStr);
        downloadAnchorNode.setAttribute("download", "all-keypairs.json");
        document.body.appendChild(downloadAnchorNode); // required for firefox
        downloadAnchorNode.click();
        downloadAnchorNode.remove();
    };

    // New Handlers for Deployment
    const handleSolidityFileUpload = (e) => {
        const file = e.target.files[0];
        if (file && file.name.endsWith('.sol')) {
            const reader = new FileReader();
            reader.onload = (event) => {
                setSolidityContent(event.target.result);
                setSolidityFileName(file.name); // Set after reading
                setDeployError(''); // Reset error message
            };
            reader.readAsText(file);
        } else {
            setSolidityFileName('');
            setSolidityContent('');
            setDeployError('Please upload a valid Solidity (.sol) file.');
            if (solidityFileInputRef.current) {
                solidityFileInputRef.current.value = '';
            }
        }
    };

    const handleDeployJsonFileUpload = (e) => {
        const file = e.target.files[0];
        if (file && file.type === 'application/json') {
            const reader = new FileReader();
            reader.onload = (event) => {
                try {
                    const json = JSON.parse(event.target.result);
                    // Validate JSON data
                    if (typeof json.arguments === 'string' && typeof json.contract_name === 'string') {
                        setDeployJsonContent(json);
                        setDeployError('');
                        setDeployJsonFileName(file.name); // Set file name after validation
                    } else {
                        setDeployJsonContent(null);
                        setDeployError('Invalid JSON: Missing "arguments" and "contract_name"');
                        if (deployJsonFileInputRef.current) {
                            deployJsonFileInputRef.current.value = '';
                        }
                    }
                } catch (err) {
                    console.error('Error parsing JSON:', err);
                    setDeployJsonContent(null);
                    setDeployError('Invalid JSON format.');
                    if (deployJsonFileInputRef.current) {
                        deployJsonFileInputRef.current.value = '';
                    }
                }
            };
            reader.readAsText(file);
        } else {
            setDeployJsonFileName('');
            setDeployJsonContent(null);
            setDeployError('Please upload a valid JSON file.');
            if (deployJsonFileInputRef.current) {
                deployJsonFileInputRef.current.value = '';
            }
        }
    };

    // New Handlers for Deployment Drag and Drop
    const handleDeployDragEnter = (e) => {
        e.preventDefault();
        e.stopPropagation();
    };

    const handleDeployDragOver = (e) => {
        e.preventDefault();
        e.stopPropagation();
    };

    const handleDeployDrop = (e) => {
        e.preventDefault();
        e.stopPropagation();
        const file = e.dataTransfer.files[0];
        if (file && file.type === 'application/json') {
            const reader = new FileReader();
            reader.onload = (event) => {
                try {
                    const json = JSON.parse(event.target.result);
                    // Validate JSON data
                    if (typeof json.arguments === 'string' && typeof json.contract_name === 'string') {
                        setDeployJsonContent(json);
                        setDeployError('');
                        setDeployJsonFileName(file.name); // Set file name after validation
                    } else {
                        setDeployJsonContent(null);
                        setDeployError('Invalid JSON format: "arguments" and "contract_name" are required.');
                        if (deployJsonFileInputRef.current) {
                            deployJsonFileInputRef.current.value = '';
                        }
                    }
                } catch (err) {
                    console.error('Error parsing JSON:', err);
                    setDeployJsonContent(null);
                    setDeployError('Invalid JSON format.');
                    if (deployJsonFileInputRef.current) {
                        deployJsonFileInputRef.current.value = '';
                    }
                }
            };
            reader.readAsText(file);
        } else {
            setDeployJsonFileName('');
            setDeployJsonContent(null);
            setDeployError('Please upload a valid JSON file.');
            if (deployJsonFileInputRef.current) {
                deployJsonFileInputRef.current.value = '';
            }
        }
    };

    // Handler functions for Solidity Drag and Drop
    const handleSolidityDragEnter = (e) => {
        e.preventDefault();
        e.stopPropagation();
    };

    const handleSolidityDragOver = (e) => {
        e.preventDefault();
        e.stopPropagation();
    };

    const handleSolidityDrop = (e) => {
        e.preventDefault();
        e.stopPropagation();
        const file = e.dataTransfer.files[0];
        if (file && file.name.endsWith('.sol')) {
            const reader = new FileReader();
            reader.onload = (event) => {
                setSolidityContent(event.target.result);
                setSolidityFileName(file.name); // Set after reading
                setDeployError(''); // Reset error message
            };
            reader.readAsText(file);
        } else {
            setSolidityFileName('');
            setSolidityContent('');
            setDeployError('Please upload a valid Solidity (.sol) file.');
            if (solidityFileInputRef.current) {
                solidityFileInputRef.current.value = '';
            }
        }
    };

    const handleFileClick = () => {
        fileInputRef.current.click();
    };

    const handleKeyPairFileClick = () => {
        keyPairFileInputRef.current.click();
    };

    // New Handlers for Deployment
    const handleDeployFileClick = () => {
        deployJsonFileInputRef.current.click();
    };

    const deployJsonFileInputRef = useRef(null);
    const solidityFileInputRef = useRef(null);

    // New Handler for Deployment
    const handleDeploy = () => {
        // Validate that both Solidity and JSON files are uploaded
        if (!solidityContent || !deployJsonContent) {
            setDeployError('Both Solidity contract and JSON configuration files are required.');
            return;
        }

        // Ensure JSON has required fields
        const { arguments: args, contract_name } = deployJsonContent;
        if (!args || !contract_name) {
            setDeployError('JSON file must contain "arguments" and "contract_name".');
            return;
        }

        if (!isConnected) {
            setDeployError('Please connect to a net before deploying a contract.');
            return;
        }

        // Send deployment data to background script
        chrome.runtime.sendMessage({
            action: 'deployContractChain',
            soliditySource: solidityContent,
            deployConfig: {
                arguments: args,
                contract_name: contract_name
            },
            ownerAddress: ownerAddress,
            domain: domain,
            net: selectedNet
        }, (response) => {
            if (response.success) {
                if (response.contractAddress) {
                    setContractAddress(response.contractAddress);
                    setShowContractModal(true);
                    // Clear the uploaded files
                    setSolidityFileName('');
                    setSolidityContent('');
                    setDeployJsonFileName('');
                    setDeployJsonContent(null);
                    setDeployError('');

                    // Reset file input values
                    if (solidityFileInputRef.current) {
                        solidityFileInputRef.current.value = '';
                    }
                    if (deployJsonFileInputRef.current) {
                        deployJsonFileInputRef.current.value = '';
                    }
                } else {
                    setDeployError('Deployment succeeded but no contract address returned.');
                }
            } else {
                setDeployError(response.error || 'Contract deployment failed.');
            }
        });
    };

    // Function to handle favicon load error
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
          setSelectedKeyPairIndex(keyPairs.length - 1); // Select the newly generated key pair
          disconnectDueToKeysChange();
        });
    };

    // Function to close the alert modal
    const closeAlertModal = () => {
        setAlert({ isOpen: false, message: '' });
    };

    // Handler function for uploading key pairs
    const handleKeyPairFileUpload = (e) => {
        const file = e.target.files[0];
        if (file && file.type === 'application/json') {
            const reader = new FileReader();
            reader.onload = (event) => {
                try {
                    const json = JSON.parse(event.target.result);
                    // Assume json is an array of keyPairs or a single keyPair
                    if (Array.isArray(json)) {
                        appendKeyPairs(json);
                        setAlert({ isOpen: true, message: 'Key pairs uploaded successfully.' });
                    } else if (json.publicKey && json.privateKey) {
                        appendKeyPairs([json]);
                        setAlert({ isOpen: true, message: 'Key pair uploaded successfully.' });
                    } else {
                        setAlert({ isOpen: true, message: 'Invalid JSON format for key pair.' });
                    }
                } catch (err) {
                    console.error('Error parsing key pair JSON:', err);
                    setAlert({ isOpen: true, message: 'Invalid JSON format for key pair.' });
                }
            };
            reader.readAsText(file);
        } else {
            setAlert({ isOpen: true, message: 'Please upload a valid JSON file.' });
        }
    };

    const handleAddressClick = () => {
        try {
        const address = contractAddress;
        const tempInput = document.createElement('input');
        tempInput.value = address;
        document.body.appendChild(tempInput);
        tempInput.select();
        document.execCommand('copy');
        document.body.removeChild(tempInput);
        setIsAddressCopied(true);
        setTimeout(() => {
            setIsAddressCopied(false);
        }, 1500);
        } catch (err) {
        setDeployError('Unable to copy contract address.');
        console.error('Unable to copy text: ', err);
        }
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
                <span className="badge" onClick={toggleServiceMode}>
                    {serviceMode === 'Contract' ? 'Smart Contract' : 'Key-Value'}
                </span>
                </div>
                <div className="header__icon open-panel">
                <button
                    style={{ background: 'none', color: 'white', fontWeight: 'bolder', outline: 'none', borderStyle: 'none', cursor: 'pointer' }}
                    onClick={back}
                    title="Logout"
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
                                <button className="icon-button" onClick={() => removeNet(net.name)} title="Delete Net">
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
                        <button onClick={addNet} className="button-save" title="Save Net">
                            Save
                        </button>
                        <button onClick={() => setShowModal(false)} className="button-close" title="Close Modal">
                            Close
                        </button>
                    </div>
                    {error && <p className="error-message">{error}</p>}
                </div>
                </div>
            </div>
            )}

            {/* Alert Modal */}
            {alert.isOpen && (
                <div className="overlay">
                    <div className="modal">
                        <div className="modal-content">
                            <h2>Alert</h2>
                            <p>{alert.message}</p>
                            <div className="save-container">
                                <button onClick={closeAlertModal} className="button-save" title="Okay">
                                    Okay
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            )}

            {/* Contract Address Modal */}
            {showContractModal && (
                <div className="overlay">
                    <div className="modal">
                        <div className="modal-content">
                            <h2>Contract Deployed Successfully!</h2>
                            <p>Contract Address:</p>
                            <div className="fieldset">
                                <div className="radio-option radio-option--full">
                                    <input
                                    type="radio"
                                    name="contractAddress"
                                    id="contractAddress"
                                    value={contractAddress}
                                    checked
                                    readOnly
                                    onClick={handleAddressClick}
                                    />
                                    <label htmlFor="contractAddress">
                                    <span>{isAddressCopied ? 'Copied' : `${contractAddress.slice(0, 5)}...${contractAddress.slice(-5)}`}</span>
                                    </label>
                                </div>
                            </div>
                            <button onClick={() => setShowContractModal(false)} className="button-close" title="Close Modal">
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
                {selectedNet === 'Custom URL' && (
                    <input
                    type="text"
                    value={customUrl}
                    onChange={handleCustomUrlChange}
                    placeholder="Enter custom URL"
                    className="input"
                    />
                )}
                </div>

                {/* Deploy Contract Section */}
                <div className="deploy-section">
                    <div className="file-upload-row">
                        {/* Solidity Contract Upload */}
                        <div className="file-upload">
                            <div
                                className={`drag_box_outline ${solidityFileName ? 'file-uploaded' : ''}`}
                                onDragEnter={handleSolidityDragEnter}
                                onDragOver={handleSolidityDragOver}
                                onDrop={handleSolidityDrop}
                                onClick={() => solidityFileInputRef.current.click()}
                            >
                                <input
                                    type="file"
                                    ref={solidityFileInputRef}
                                    style={{ display: 'none' }}
                                    accept=".sol"
                                    onChange={handleSolidityFileUpload}
                                />
                                {solidityFileName ? (
                                    <span className="filename">{solidityFileName} uploaded</span>
                                ) : (
                                    <span className="filename">Contract (.sol)</span>
                                )}
                            </div>
                        </div>

                        {/* JSON Configuration Upload */}
                        <div className="file-upload">
                            <div
                                className={`drag_box_outline ${deployJsonFileName ? 'file-uploaded' : ''}`}
                                onDragEnter={handleDeployDragEnter}
                                onDragOver={handleDeployDragOver}
                                onDrop={handleDeployDrop}
                                onClick={() => deployJsonFileInputRef.current.click()}
                            >
                                <input
                                    type="file"
                                    ref={deployJsonFileInputRef}
                                    style={{ display: 'none' }}
                                    accept="application/json"
                                    onChange={handleDeployJsonFileUpload}
                                />
                                {deployJsonFileName ? (
                                    <span className="filename">{deployJsonFileName} uploaded</span>
                                ) : (
                                    <span className="filename">Configuration (.json)</span>
                                )}
                            </div>
                        </div>
                    </div>
                    {deployError && <p className="error-message">{deployError}</p>}
                </div>
            </div>


            <h2 className="page__title">Select Account</h2>
            <div className="net">
                <div className="keypair">
                    <div className="keypair-header">
                        <div className="select-wrapper">
                            <select
                            value={selectedKeyPairIndex}
                            onChange={(e) => switchKeyPair(Number(e.target.value))}
                            className="select"
                            >
                            {keyPairs.map((keyPair, index) => {
                                const ownerAddr = generateOwnerAddress(keyPair.publicKey);
                                return (
                                <option key={index} value={index}>
                                    {`${ownerAddr.slice(0, 4)}...${ownerAddr.slice(-4)}`}
                                </option>
                                );
                            })}
                            </select>
                            <i className="fas fa-chevron-down"></i>
                        </div>
                        <div className="keypair-icons">
                            {keyPairs.length > 1 && (
                            <button onClick={() => setShowDeleteModal(true)} className="icon-button" title="Delete Key Pair">
                                <DeleteIcon style={{ color: 'white' }} />
                            </button>
                            )}
                            <button onClick={handleCopyOwnerAddress} className="icon-button" title="Copy Owner Address">
                                <ContentCopyIcon style={{ color: isCopied ? 'grey' : 'white' }} />
                            </button>
                            <button onClick={handleDownloadKeyPair} className="icon-button" title="Download Key Pair">
                                <DownloadIcon style={{ color: 'white' }} />
                            </button>
                        </div>
                    </div>
                    <div className="keypair-actions">
                        <div className="button-with-tooltip">
                            <button onClick={handleGenerateKeyPair} className="badge-button centered-icon" title="Generate Keys">
                                <AddCircleOutlineIcon style={{ color: 'white' }} />
                            </button>
                            <span className="tooltip-text">Generate Keys</span>
                        </div>
                        <div className="button-with-tooltip">
                            <button onClick={handleDownloadAllKeyPairs} className="badge-button centered-icon" title="Export All Keys">
                                <SaveAltIcon style={{ color: 'white' }} />
                            </button>
                            <span className="tooltip-text">Export All Keys</span>
                        </div>
                        <div className="button-with-tooltip">
                            <button onClick={handleKeyPairFileClick} className="badge-button centered-icon" title="Upload Keys">
                                <UploadIcon style={{ color: 'white' }} />
                            </button>
                            <span className="tooltip-text">Upload Keys</span>
                        </div>
                        <input
                            type="file"
                            ref={keyPairFileInputRef}
                            style={{ display: 'none' }}
                            accept="application/json"
                            onChange={handleKeyPairFileUpload}
                        />
                    </div>
                </div>
            </div>

            {showDeleteModal && (
                <div className="modal-overlay">
                    <div className="modal">
                        <div className="modal-content">
                            <h2>Are you sure?</h2>
                            <p>This action is irreversible and will delete the selected key pair forever.</p>
                            <div className="save-container">
                                <button onClick={handleDeleteKeyPair} className="button-save" title="Delete Key Pair">
                                    Delete
                                </button>
                                <button onClick={() => setShowDeleteModal(false)} className="button-close" title="Cancel">
                                    Cancel
                                </button>
                            </div>
                        </div>
                    </div>
                </div>
            )}

            {/* Deploy Button */}
            <button
                className="button button--full button--main open-popup"
                onClick={handleDeploy}
                title="Deploy Contract"
                disabled={!solidityContent || !deployJsonContent || !isConnected}
            >
                Deploy
            </button>

            <p className="bottom-navigation" style={{ backgroundColor: 'transparent', display: 'flex', justifyContent: 'center', textShadow: '1px 1px 1px rgba(0, 0, 0, 0.3)', color: 'rgb(255, 255, 255, 0.5)', fontSize: '9px' }}>
                ResVault v{versionData.version}
            </p>
            </div>
        </div>
        </>
    );

}

export default Contract;