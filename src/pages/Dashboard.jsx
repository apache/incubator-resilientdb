/*global chrome*/
import logo from '../logo.svg';
import '../App.css';
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";

function Dashboard(props) {
  const location = useLocation();
  
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
    }

    else if ((msg.from === 'update')){
      const store = location.state;
      store.id = msg.id;
      store.address = msg.address;
      store.amount = msg.amount;
      store.data = msg.data;
      store.from = msg.from;
      store.operation = "UPDATE";
      props.navigate("/transaction", {state: store} );
    }

    else if ((msg.from === 'update-multi')){
      const store = location.state;
      const receivedValuesList = msg.values;
      store.values = receivedValuesList;
      console.log(store.values);
      store.from = msg.from;
      store.operation = "UPDATE MULTIPLE";
      props.navigate("/transaction", {state: store} );
    }

    else if ((msg.from === 'filter')){
      const store = location.state;
      store.ownerPublicKey = msg.ownerPublicKey;
      store.recipientPublicKey = msg.recipientPublicKey;
      store.from = msg.from;
      props.navigate("/transaction", {state: store} );
    }

    else if ((msg.from === 'account')){
      const store = location.state;
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
            <button className="buttonSignOut" data-inline="true" onClick={back}><ExitToAppIcon /></button>
          </div>
        </div>
      </div>

      <div className="paymentBottomDashboard vcenter">
        <p className="manifest">
          <ul className="list">
            <li className="list"><span className="list"></span></li>
          </ul>
        </p>
      </div>

      <div className="paymentBottomDashboardBack vcenter">
        <button className="buttonCreate center" data-inline="true"> Submit </button>
      </div>

      <div className="footerLogin">
            <p className="special"><b>Account:</b> {location.state.publicKey}</p>
      </div>
    </div>
  </div>
  )
}

export default Dashboard;