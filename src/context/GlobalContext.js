/*global chrome*/
import React, { createContext, useState, useEffect } from 'react';
import CryptoJS from 'crypto-js';
import nacl from 'tweetnacl';
import Base58 from 'bs58';

export const GlobalContext = createContext();

export const GlobalProvider = ({ children }) => {
  const [values, setValues] = useState({ password: '', showPassword: false });
  const [confirmValues, setConfirmValues] = useState({ password: '', showPassword: false });
  const [loginValues, setLoginValues] = useState({ password: '', showPassword: false });
  const [publicKey, setPublicKey] = useState('');
  const [privateKey, setPrivateKey] = useState('');
  const [keyPairs, setKeyPairs] = useState([]);
  const [selectedKeyPairIndex, setSelectedKeyPairIndex] = useState(0);
  const [isAuthenticated, setIsAuthenticated] = useState(false);
  const [storedPassword, setStoredPassword] = useState('');

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
          callback([]);
        }
      } else {
        callback([]);
      }
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
      return;
    }

    const keyPair = nacl.sign.keyPair();
    const newPublicKey = Base58.encode(keyPair.publicKey);
    const newPrivateKey = Base58.encode(keyPair.secretKey.slice(0, 32));
    const newKeyPair = { publicKey: newPublicKey, privateKey: newPrivateKey };

    // Load existing key pairs
    loadKeyPairsFromStorage(password, (existingKeyPairs) => {
      const updatedKeyPairs = [...existingKeyPairs, newKeyPair];
      // Save updated key pairs
      saveKeyPairsToStorage(updatedKeyPairs, password);
      setKeyPairs(updatedKeyPairs);
      setPublicKey(newPublicKey);
      setPrivateKey(newPrivateKey);
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

  // Load key pairs from storage when context is initialized
  useEffect(() => {
    // Retrieve password from storage
    chrome.storage.local.get(['password'], (result) => {
      const password = result.password;
      if (password) {
        setStoredPassword(password);
        loadKeyPairsFromStorage(password, (loadedKeyPairs) => {
          if (loadedKeyPairs.length > 0) {
            setKeyPairs(loadedKeyPairs);
            // Load selected key pair index
            loadSelectedKeyPairIndex((index) => {
              if (loadedKeyPairs[index]) {
                setPublicKey(loadedKeyPairs[index].publicKey);
                setPrivateKey(loadedKeyPairs[index].privateKey);
                setSelectedKeyPairIndex(index);
              } else if (loadedKeyPairs.length > 0) {
                setPublicKey(loadedKeyPairs[0].publicKey);
                setPrivateKey(loadedKeyPairs[0].privateKey);
                setSelectedKeyPairIndex(0);
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
  const setSelectedKeyPair = (index) => {
    if (keyPairs[index]) {
      setPublicKey(keyPairs[index].publicKey);
      setPrivateKey(keyPairs[index].privateKey);
      setSelectedKeyPairIndex(index);
      saveSelectedKeyPairIndex(index);

      const password = storedPassword;
      if (!password) {
        console.error('Password is not available');
        return;
      }
      // Encrypt the private key and save in 'store'
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

  return (
    <GlobalContext.Provider
      value={{
        values,
        setValues,
        confirmValues,
        setConfirmValues,
        loginValues,
        setLoginValues,
        publicKey,
        setPublicKey,
        privateKey,
        setPrivateKey,
        keyPairs,
        setKeyPairs,
        generateKeyPair,
        selectedKeyPairIndex,
        setSelectedKeyPairIndex,
        setSelectedKeyPair,
        isAuthenticated,
        setIsAuthenticated,
        storedPassword,
        setStoredPassword,
      }}
    >
      {children}
    </GlobalContext.Provider>
  );
};