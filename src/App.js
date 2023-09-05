/*global chrome*/
import './App.css';
import React, { useState } from 'react';
import Home from "./pages/Home";
import Login from "./pages/Login";
import Dashboard from "./pages/Dashboard";
import Transaction from "./pages/Transaction";
import Logs from "./pages/Logs";
import { Navigate, Route, Routes } from "react-router-dom";
import { useNavigate } from "react-router-dom";

function App() {
  const navigate = useNavigate();
  const [isLoading, setLoading] = useState(true);
  const [publicKey, setPublicKey] = useState("");
  const [privateKey, setPrivateKey] = useState("");
  const [encryptedPrivateKey, setEncryptedPrivateKey] = useState("");
  const [hash, setHash] = useState("");
  const [operation, setOperation] = useState("");
  const [amount, setAmount] = useState("");
  const [data, setData] = useState("");
  const [accountAddress, setAccountAddress] = useState("");
  const [componentAddress, setComponentAddress] = useState("");
  const [footer, setFooter] = useState("");
  const [values, setValues] = useState({
    password: "",
    showPassword: false,
  })
  const [loginValues, setLoginValues] = useState({
    password: "",
    showPassword: false,
  })
  const [confirmValues, setConfirmValues] = useState({
    password: "",
    showPassword: false,
  })

  const props = {
    navigate,
    isLoading,
    publicKey,
    privateKey,
    encryptedPrivateKey,
    hash,
    operation,
    amount,
    data,
    accountAddress,
    componentAddress,
    values,
    loginValues,
    confirmValues
  }

  const propsChange = {
    setLoading,
    setPublicKey,
    setPrivateKey,
    setEncryptedPrivateKey,
    setHash,
    setOperation,
    setAmount,
    setData,
    setAccountAddress,
    setComponentAddress,
    setValues,
    setLoginValues,
    setConfirmValues
  }

  return (
    <Routes>
      <Route path='/' element={<Home {...props} {...propsChange} />} />
      <Route path='/login' element={<Login {...props} {...propsChange} />} />
      <Route path='/dashboard' element={<Dashboard {...props} {...propsChange} />} />
      <Route path='/transaction' element={<Transaction {...props} {...propsChange} />} />
      <Route path='/logs' element={<Logs {...props} {...propsChange} />} />
      <Route path='*' element={<Navigate to='/' />} />
    </Routes>
  )
}

export default App;
