/*global chrome*/
import React, { createContext, useState, useEffect } from 'react';
import CryptoJS from 'crypto-js';

export const GlobalContext = createContext();

export const GlobalProvider = ({ children }) => {
  const [values, setValues] = useState({ password: '', showPassword: false });
  const [loginValues, setLoginValues] = useState({ password: '', showPassword: false });
  const [confirmValues, setConfirmValues] = useState({ password: '', showPassword: false });
  const [transactionData, setTransactionData] = useState({});
  const [publicKey, setPublicKey] = useState('');
  const [privateKey, setPrivateKey] = useState('');
  const [networks, setNetworks] = useState([]);

  useEffect(() => {
    // Retrieve publicKey and encryptedPrivateKey from storage on initialization
    chrome.storage.sync.get('store', (data) => {
      if (data.store && data.store.publicKey) {
        setPublicKey(data.store.publicKey);
        chrome.storage.local.get('password', (passwordData) => {
          if (passwordData.password) {
            const decryptedPrivateKey = CryptoJS.AES.decrypt(
              data.store.encryptedPrivateKey,
              passwordData.password
            ).toString(CryptoJS.enc.Utf8);
            setPrivateKey(JSON.parse(decryptedPrivateKey));
          }
        });
      }
    });
  }, []);

  const updateTransaction = (data) => setTransactionData(data);
  const clearData = () => setTransactionData({});

  return (
    <GlobalContext.Provider
      value={{
        values,
        setValues,
        loginValues,
        setLoginValues,
        confirmValues,
        setConfirmValues,
        transactionData,
        updateTransaction,
        clearData,
        publicKey,
        setPublicKey,
        privateKey,
        setPrivateKey,
        networks,
        setNetworks,
      }}
    >
      {children}
    </GlobalContext.Provider>
  );
};