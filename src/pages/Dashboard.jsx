/*global chrome*/
import logo from '../logo.png';
import '../App.css';
import Footer from "../components/Footer";
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";

function Dashboard(props) {
  const location = useLocation();
  props.setFooter("footerLogin");
  
  chrome.runtime.onMessage.addListener((msg, sender, sendResponse) => {
    if ((msg.from === 'commit')) {
      const store = location.state;
      store.address = msg.address;
      store.amount = msg.amount;
      store.data = msg.data;
      store.from = msg.from;
      store.operation = "CREATE";
      props.navigate("/transaction", {state: store} );
    }

    else if ((msg.from === 'get')){
      const store = location.state;
      store.id = msg.id;
      store.from = msg.from;
      props.navigate("/transaction", {state: store} );
    };
  });

  const back = async () => {
    const store = location.state;
    chrome.storage.local.clear(function(){
      props.navigate("/login", {state: store});
    });
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
            <p className="publicKeyStyle" style={{color: 'white'}}><b>Account:</b> {location.state.publicKey}</p>
            <button className="buttonSignOut" data-inline="true" onClick={back}><ExitToAppIcon /></button>
          </div>
        </div>
      </div>

      <div className="paymentBottomDashboard vcenter">
        <textarea className="scrollabletextbox" rows="10" cols="45">
        </textarea>
      </div>

      <div className="paymentBottomDashboardBack vcenter">
        <button className="buttonCreate center" data-inline="true"> Submit </button>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Dashboard;