import logo from './logo.png';
import './App.css';
import axios from 'axios';
import React, { useState } from 'react';


function App() {
  const [publicKey, setPublicKey] = useState("");
  const [publicKeyDisplay, setPublicKeyDisplay] = useState(false);
  const createAccount = async () => {
  
    const headers = {
        'Content-Type': 'application/json',
        'Accept': '*/*'
    };
  
    const data = {
      query: `mutation {
        generateKeys {
          publicKey
          privateKey
        }
      }`
    }
    axios
      .post("http://localhost:8000/graphql", data, { headers })
      .then(function (response) {
        const res = response.data; // Response received from the API
        const getPublicKey = res.data.generateKeys.publicKey;
        setPublicKeyDisplay(true);
        setPublicKey(getPublicKey);
      })
      .catch(function (error) {
        console.error(error);
      });
  };

  return (
    <div className="App">
      <div className="mainContainer">
        <div className="cardHolder">
          <div className="header">
            <div className="heading center">Global-Scale Blockchain Fabric</div>
            <div className="stepHeading center">NexRes Wallet</div>
            <div className="logo">
              <img src={logo} alt="logo" />
            </div>
          </div>
        </div>
      
        <div className="payment vcenter">
          <button className="buttonCreate center" onClick={createAccount}>Create Account</button>
        </div>

        
        {publicKeyDisplay &&
        <div>
          <div className='paymentNew vcenter'> 
            <div>Public Key</div> 
          </div> 
          <div className='paymentKey vcenter'> 
            <div className='publicKey'>{publicKey}</div>
          </div>
        </div>
        }

        <div className="footer">
          <p>&copy; 2023, Alpha release v0.1</p>
        </div>
      </div>
    </div>
  );
}

export default App;
