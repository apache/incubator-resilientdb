/*global chrome*/
import logo from '../logo.png';
import '../App.css';
import Footer from "../components/Footer";
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";
import { sendRequest } from '../client';
import { useEffect } from 'react';

function Logs(props) {
  useEffect(() => {
  })

  const location = useLocation();
  props.setFooter("footerLogin");

  const back = async () => {
    const store = location.state;
    chrome.storage.local.clear(function(){
      props.navigate("/login", {state: store});
    });
  }

  const dashboard = async () => {
    props.navigate("/dashboard", {state: location.state});
  }
 
  return (
  <div className="App">
    <div className="mainContainer">
      <div className="cardHolder">
        <div className="header">
          <div className="heading center">
            Global-Scale Blockchain Fabric
          </div> 
          <div className="stepHeading center">NexRes Wallet</div>
          <div className="logo">
            <img src={logo} alt="logo" />
          </div>
          <div className="paymentTopKey vcenter">
            <p className="publicKeyStyle" style={{color: 'white'}}><b>ID:</b> {location.state.publicKey}</p>
            <button className="buttonSignOut" data-inline="true" onClick={back}><ExitToAppIcon /></button>
          </div>
        </div>
      </div>

      <div className="paymentBottomDashboard vcenter">
        <p className="publicKeyStyle">{location.state.id}</p>
      </div>

      <div className="paymentBottomDashboardBack vcenter">
        <button className="buttonCreate center" data-inline="true" onClick={dashboard}> Back </button>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Logs;