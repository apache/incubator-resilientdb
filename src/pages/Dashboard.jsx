/*global chrome*/
import logo from '../logo.png';
import '../App.css';
import Footer from "../components/Footer";
import ExitToAppIcon from '@mui/icons-material/ExitToApp';
import { useLocation } from "react-router-dom";
import { sendRequest } from '../client';

function Dashboard(props) {
  const location = useLocation();
  props.setFooter("footerLogin");
  
  chrome.runtime.onMessage.addListener((msg, sender) => {
    if ((msg.from === 'commit')) {
      var escapeCodes = { 
          '\\': '\\',
          'r':  '\r',
          'n':  '\n',
          't':  '\t'
      };
      msg.data.replace(/\\(.)/g, function(str, char) {
        return escapeCodes[char];
      });

      const query = `mutation {
        postTransaction(data: {
          operation: "CREATE",
          amount: ${parseInt(msg.amount)},
          signerPublicKey: "${location.state.publicKey}",
          signerPrivateKey: "${location.state.privateKey}",
          recipientPublicKey: "${msg.address}",
          asset: """{
              "data": { 
                ${msg.data}
              },
          }
          """
        }){
          id
        }
      }`
  
      const result = sendRequest(query).then(res => { 
        const store = location.state;
        store.id = res.data.postTransaction.id;
        console.log(res.data.postTransaction);
        props.navigate("/logs", {state: store});
      });
    }

    else if ((msg.from === 'get')){
      const query = `query {
        getTransaction(id: "${msg.id}"){
          id
          version
          amount
          metadata
          operation
          asset
          publicKey
          uri
          type
        }
      }`
  
      const result = sendRequest(query).then(res => { 
        console.log(res.data.getTransaction);
      });
    }
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