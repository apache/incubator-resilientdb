import './App.css';
import React, { useState } from 'react';
import Home from "./pages/Home";

function App() {
  const [publicKey, setPublicKey] = useState("");
  const [password, setPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [publicKeyDisplay, setPublicKeyDisplay] = useState(false);
  const props = {
    publicKey,
    publicKeyDisplay,
    password,
    confirmPassword
  }

  const propsChange = {
    setPublicKey,
    setPublicKeyDisplay,
    setPassword,
    setConfirmPassword
  }

  return (
    <>
      <Home {...props} {...propsChange} />
    </>
  )
}

export default App;
