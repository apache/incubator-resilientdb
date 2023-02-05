import './App.css';
import React, { useState } from 'react';
import Home from "./pages/Home";

function App() {
  const [publicKey, setPublicKey] = useState("");
  const [publicKeyDisplay, setPublicKeyDisplay] = useState(false);
  const [values, setValues] = useState({
    password: "",
    showPassword: false,
  })
  const [confirmValues, setConfirmValues] = useState({
    password: "",
    showPassword: false,
  })

  const props = {
    publicKey,
    publicKeyDisplay,
    values,
    confirmValues
  }

  const propsChange = {
    setPublicKey,
    setPublicKeyDisplay,
    setValues,
    setConfirmValues
  }

  return (
    <>
      <Home {...props} {...propsChange} />
    </>
  )
}

export default App;
