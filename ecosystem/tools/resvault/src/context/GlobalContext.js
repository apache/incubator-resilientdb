/*
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
*
*/


/*global chrome*/
import React, { createContext, useState, useEffect } from 'react';
import CryptoJS from 'crypto-js';
import { keccak256 } from 'js-sha3';
import nacl from 'tweetnacl';
import Base58 from 'bs58';
import { useNavigate } from 'react-router-dom';

export const GlobalContext = createContext();

export const GlobalProvider = ({ children }) => {
  const [serviceMode, setServiceMode] = useState('KV'); // KV or Contract
  const [values, setValues] = useState({ password: '', showPassword: false });
  const [confirmValues, setConfirmValues] = useState({ password: '', showPassword: false });
  const [loginValues, setLoginValues] = useState({ password: '', showPassword: false });
  const [publicKey, setPublicKey] = useState('');
  const [privateKey, setPrivateKey] = useState('');
  const [ownerAddress, setOwnerAddress] = useState(''); // To store the generated owner's address
  const [keyPairs, setKeyPairs] = useState([]);
  const [selectedKeyPairIndex, setSelectedKeyPairIndex] = useState(0);
  const [isAuthenticated, setIsAuthenticated] = useState(false);
  const [storedPassword, setStoredPassword] = useState('');

  const navigate = useNavigate();
  
  // Alert state for modals
  const [alert, setAlert] = useState({ isOpen: false, message: '' });

  // Generate owner's address
  const generateOwnerAddress = (publicKey) => {
    const decodedPublicKey = Base58.decode(publicKey);
    const addressHash = keccak256(Buffer.from(decodedPublicKey));
    const address = `0x${addressHash.slice(-40)}`;
    return address;
  };

  // Function to encrypt and store key pairs
  const saveKeyPairsToStorage = (keyPairs, password) => {
    const encryptedKeyPairs = CryptoJS.AES.encrypt(
      JSON.stringify(keyPairs),
      password
    ).toString();
    chrome.storage.sync.set({ encryptedKeyPairs }, () => {
      console.log('Encrypted key pairs saved to storage.');
    });
  };

  // Function to load and decrypt key pairs
  const loadKeyPairsFromStorage = (password, callback) => {
    chrome.storage.sync.get(['encryptedKeyPairs'], (result) => {
      if (result.encryptedKeyPairs) {
        try {
          const bytes = CryptoJS.AES.decrypt(result.encryptedKeyPairs, password);
          const decryptedData = bytes.toString(CryptoJS.enc.Utf8);
          const decryptedKeyPairs = JSON.parse(decryptedData);
          callback(decryptedKeyPairs);
        } catch (err) {
          console.error('Error decrypting key pairs:', err);
          setAlert({ isOpen: true, message: 'Failed to decrypt key pairs. Please check your password.' });
          callback([]);
        }
      } else {
        callback([]);
      }
    });
  };

  // Function to append new key pairs while preventing duplicates and validating them
  const appendKeyPairs = (newKeyPairs) => {
    const password = storedPassword;
    if (!password) {
        console.error('Password is not available');
        setAlert({ isOpen: true, message: 'Password is not available. Please log in again.' });
        return;
    }

    // Validate each key pair
    for (let i = 0; i < newKeyPairs.length; i++) {
        const keyPair = newKeyPairs[i];
        if (!keyPair.publicKey || !keyPair.privateKey) {
            setAlert({ isOpen: true, message: `Key pair at index ${i} is missing publicKey or privateKey.` });
            console.error(`Key pair at index ${i} is missing publicKey or privateKey.`);
            return;
        }

        try {
            // Decode private key from Base58
            const decodedPrivateKey = Base58.decode(keyPair.privateKey);
            if (decodedPrivateKey.length !== 32) {
                setAlert({ isOpen: true, message: `Private key at index ${i} is not 32 bytes.` });
                console.error(`Private key at index ${i} is not 32 bytes.`);
                return;
            }

            // Derive public key from private key using nacl.sign.keyPair.fromSeed
            const derivedKeyPair = nacl.sign.keyPair.fromSeed(decodedPrivateKey);
            const derivedPublicKey = Base58.encode(derivedKeyPair.publicKey);

            // Compare derived public key with provided public key
            if (derivedPublicKey !== keyPair.publicKey) {
                setAlert({ isOpen: true, message: `Public key does not match private key at index ${i}.` });
                console.error(`Public key does not match private key at index ${i}.`);
                return;
            }
        } catch (err) {
            console.error('Error validating key pair:', err);
            setAlert({ isOpen: true, message: `Error validating key pair at index ${i}.` });
            return;
        }
    }

    // Load existing key pairs
    loadKeyPairsFromStorage(password, (existingKeyPairs) => {
        // Filter out duplicates
        const uniqueNewKeyPairs = newKeyPairs.filter(newKey => {
            return !existingKeyPairs.some(existingKey =>
                existingKey.publicKey === newKey.publicKey &&
                existingKey.privateKey === newKey.privateKey
            );
        });

        if (uniqueNewKeyPairs.length === 0) {
            console.log('No new unique key pairs to add.');
            setAlert({ isOpen: true, message: 'No new unique key pairs to add.' });
            return;
        }

        const updatedKeyPairs = [...existingKeyPairs, ...uniqueNewKeyPairs];
        saveKeyPairsToStorage(updatedKeyPairs, password);
        setKeyPairs(updatedKeyPairs);

        // Update selected key pair to the last one added
        const newIndex = updatedKeyPairs.length - 1;
        setSelectedKeyPairIndex(newIndex);
        setPublicKey(updatedKeyPairs[newIndex].publicKey);
        setPrivateKey(updatedKeyPairs[newIndex].privateKey);
        saveSelectedKeyPairIndex(newIndex);

        // Generate and set the ownerAddress
        const ownerAddress = generateOwnerAddress(updatedKeyPairs[newIndex].publicKey);
        setOwnerAddress(ownerAddress);

        // Update 'store' with the new key pair
        const encryptedPrivateKey = CryptoJS.AES.encrypt(updatedKeyPairs[newIndex].privateKey, password).toString();
        const hash = CryptoJS.SHA256(password).toString(CryptoJS.enc.Hex);
        const store = {
            hash,
            publicKey: updatedKeyPairs[newIndex].publicKey,
            encryptedPrivateKey: encryptedPrivateKey,
            history: [],
        };
        chrome.storage.sync.set({ store }, () => {
            console.log('Store updated with new key pair');
        });
    });
  };

  // Function to save selected key pair index
  const saveSelectedKeyPairIndex = (index) => {
    chrome.storage.local.set({ selectedKeyPairIndex: index }, () => {});
  };

  // Function to load selected key pair index
  const loadSelectedKeyPairIndex = (callback) => {
    chrome.storage.local.get(['selectedKeyPairIndex'], (result) => {
      const index = result.selectedKeyPairIndex !== undefined ? result.selectedKeyPairIndex : 0;
      callback(index);
    });
  };

  // Function to generate new key pair and store it
  const generateKeyPair = (callback) => {
    const password = storedPassword;
    if (!password) {
      console.error('Password is not available');
      setAlert({ isOpen: true, message: 'Password is not available. Please log in again.' });
      return;
    }

    const keyPair = nacl.sign.keyPair();
    const newPublicKey = Base58.encode(keyPair.publicKey);
    const newPrivateKey = Base58.encode(keyPair.secretKey.slice(0, 32)); // Using the first 32 bytes as seed
    const ownerAddress = generateOwnerAddress(newPublicKey);
    const newKeyPair = { publicKey: newPublicKey, privateKey: newPrivateKey };

    // Load existing key pairs
    loadKeyPairsFromStorage(password, (existingKeyPairs) => {
      // Check for duplicates before adding
      const isDuplicate = existingKeyPairs.some(existingKey =>
          existingKey.publicKey === newKeyPair.publicKey &&
          existingKey.privateKey === newKeyPair.privateKey
      );

      if (isDuplicate) {
          console.log('Generated key pair is a duplicate. Skipping.');
          setAlert({ isOpen: true, message: 'Generated key pair is a duplicate. Skipping.' });
          return;
      }

      const updatedKeyPairs = [...existingKeyPairs, newKeyPair];
      // Save updated key pairs
      saveKeyPairsToStorage(updatedKeyPairs, password);
      setKeyPairs(updatedKeyPairs);
      setPublicKey(newPublicKey);
      setPrivateKey(newPrivateKey);
      setOwnerAddress(ownerAddress);
      const newIndex = updatedKeyPairs.length - 1;
      setSelectedKeyPairIndex(newIndex);
      saveSelectedKeyPairIndex(newIndex);
      setIsAuthenticated(true);

      // Encrypt the private key and save in 'store'
      const encryptedPrivateKey = CryptoJS.AES.encrypt(newPrivateKey, password).toString();
      const hash = CryptoJS.SHA256(password).toString(CryptoJS.enc.Hex);
      const store = {
        hash,
        publicKey: newPublicKey,
        encryptedPrivateKey: encryptedPrivateKey,
        history: [],
      };
      chrome.storage.sync.set({ store }, () => {
        console.log('Store updated with new key pair');
        if (callback) {
          callback();
        }
      });
    });
  };

  // Function to delete a key pair
  const deleteKeyPair = (index, callback) => {
    const password = storedPassword;
    if (!password) {
        console.error('Password is not available');
        setAlert({ isOpen: true, message: 'Password is not available. Please log in again.' });
        return;
    }

    loadKeyPairsFromStorage(password, (existingKeyPairs) => {
        if (existingKeyPairs.length <= 1) {
            console.error('Cannot delete the last remaining key pair.');
            setAlert({ isOpen: true, message: 'Cannot delete the last remaining key pair.' });
            return;
        }

        // Remove the key pair at the specified index
        const updatedKeyPairs = [...existingKeyPairs];
        updatedKeyPairs.splice(index, 1);

        // Save the updated keyPairs back to storage
        saveKeyPairsToStorage(updatedKeyPairs, password);
        setKeyPairs(updatedKeyPairs);

        // Reset to the first key pair after deletion
        setSelectedKeyPairIndex(0);
        if (updatedKeyPairs.length > 0) {
            setPublicKey(updatedKeyPairs[0].publicKey);
            setPrivateKey(updatedKeyPairs[0].privateKey);
            saveSelectedKeyPairIndex(0);

            // Generate and set the ownerAddress
            const ownerAddress = generateOwnerAddress(updatedKeyPairs[0].publicKey);
            setOwnerAddress(ownerAddress);
        }

        // Update 'store' with the new selected key pair
        if (updatedKeyPairs.length > 0) {
            const encryptedPrivateKey = CryptoJS.AES.encrypt(updatedKeyPairs[0].privateKey, password).toString();
            const hash = CryptoJS.SHA256(password).toString(CryptoJS.enc.Hex);
            const store = {
                hash,
                publicKey: updatedKeyPairs[0].publicKey,
                encryptedPrivateKey: encryptedPrivateKey,
                history: [],
            };
            chrome.storage.sync.set({ store }, () => {
                console.log('Store updated with selected key pair after deletion');
            });
        }

        // Optionally call the callback
        if (callback) {
            callback();
        }
    });
  };

  // Load key pairs from storage when context is initialized
  useEffect(() => {
    chrome.storage.local.get(['password'], (result) => {
      const password = result.password;
      if (password) {
        setStoredPassword(password);
        loadKeyPairsFromStorage(password, (loadedKeyPairs) => {
          if (loadedKeyPairs.length > 0) {
            setKeyPairs(loadedKeyPairs);
            loadSelectedKeyPairIndex((index) => {
              if (loadedKeyPairs[index]) {
                setPublicKey(loadedKeyPairs[index].publicKey);
                setPrivateKey(loadedKeyPairs[index].privateKey);
                setSelectedKeyPairIndex(index);

                // Generate and set the ownerAddress
                const ownerAddress = generateOwnerAddress(loadedKeyPairs[index].publicKey);
                setOwnerAddress(ownerAddress);

              } else if (loadedKeyPairs.length > 0) {
                setPublicKey(loadedKeyPairs[0].publicKey);
                setPrivateKey(loadedKeyPairs[0].privateKey);
                setSelectedKeyPairIndex(0);

                // Generate and set the ownerAddress
                const ownerAddress = generateOwnerAddress(loadedKeyPairs[0].publicKey);
                setOwnerAddress(ownerAddress);
              }
              setIsAuthenticated(true);
            });
          } else {
            setIsAuthenticated(false);
          }
        });
      } else {
        setIsAuthenticated(false);
      }
    });
  }, []);

  // Function to set selected key pair
  const setSelectedKeyPairFn = (index) => {
    if (keyPairs[index]) {
      setPublicKey(keyPairs[index].publicKey);
      setPrivateKey(keyPairs[index].privateKey);
      setSelectedKeyPairIndex(index);
      saveSelectedKeyPairIndex(index);

      // Generate and set the ownerAddress
      const ownerAddress = generateOwnerAddress(keyPairs[index].publicKey);
      setOwnerAddress(ownerAddress);

      const password = storedPassword;
      if (!password) {
        console.error('Password is not available');
        setAlert({ isOpen: true, message: 'Password is not available. Please log in again.' });
        return;
      }
      const encryptedPrivateKey = CryptoJS.AES.encrypt(keyPairs[index].privateKey, password).toString();
      const hash = CryptoJS.SHA256(password).toString(CryptoJS.enc.Hex);
      const store = {
        hash,
        publicKey: keyPairs[index].publicKey,
        encryptedPrivateKey: encryptedPrivateKey,
        history: [],
      };
      chrome.storage.sync.set({ store }, () => {
        console.log('Store updated with selected key pair');
      });
    }
  };

  useEffect(() => {
    // Retrieve the mode from storage when the app starts
    chrome.storage.local.get(['serviceMode'], (result) => {
      if (result.serviceMode) {
        setServiceMode(result.serviceMode);
        navigate(result.serviceMode === 'KV' ? '/dashboard' : '/contract');
      }
    });
  }, []);

  const toggleServiceMode = () => {
    const newMode = serviceMode === 'KV' ? 'Contract' : 'KV';
    setServiceMode(newMode);
    chrome.storage.local.set({ serviceMode: newMode }, () => {
      // Clear connections when mode changes
      chrome.storage.local.remove(['connections'], () => {
        navigate(newMode === 'KV' ? '/dashboard' : '/contract');
      });
    });
  };

  return (
    <GlobalContext.Provider
      value={{
        values, setValues,
        confirmValues, setConfirmValues,
        loginValues, setLoginValues,
        publicKey, setPublicKey,
        privateKey, setPrivateKey,
        keyPairs, setKeyPairs,
        generateKeyPair,
        selectedKeyPairIndex, setSelectedKeyPairIndex,
        setSelectedKeyPair: setSelectedKeyPairFn,
        isAuthenticated, setIsAuthenticated,
        storedPassword, setStoredPassword,
        deleteKeyPair,
        appendKeyPairs,
        alert, setAlert,
        serviceMode, setServiceMode, toggleServiceMode,
        ownerAddress, setOwnerAddress,
      }}
    >
      {children}
    </GlobalContext.Provider>
  );
};
