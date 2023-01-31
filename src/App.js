import './App.css';
import React, { useState } from 'react';
import Home from "./pages/Home";

function App() {
  const [publicKey, setPublicKey] = useState("");
  const [publicKeyDisplay, setPublicKeyDisplay] = useState(false);
  const props = {
    publicKey,
    publicKeyDisplay
  }

  const propsChange = {
    setPublicKey,
    setPublicKeyDisplay
  }

  return (
    <>
      <Home {...props} {...propsChange} />
      <input value={publicKey}  />
    </>
  )
}

export default App;
