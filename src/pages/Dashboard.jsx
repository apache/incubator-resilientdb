import logo from '../logo.png';
import '../App.css';
import Footer from "../components/Footer";
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import { Link } from "react-router-dom";

function Dashboard(props) {
  props.setFooter("footerLogin");
 
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

      <div className="paymentTopKey vcenter">
        <p className="publicKeyStyle"><b>Public Key:</b> {props.publicKey}</p>
      </div>
      <div className="paymentBottomKey vcenter">
        <p className="publicKeyStyle"><b>Private Key:</b> {props.privateKey}</p>
      </div>

      <div className="paymentBottomDashboard vcenter">
        <textarea className="scrollabletextbox" rows="3" cols="35"> Insert Dynamic Manifest file here</textarea>
      </div>

      <div className="paymentBottomDashboardBack vcenter">
        <button className="buttonCreate center" data-inline="true"> Submit </button>
        <Link to="/login" className="routingBack"><button className="buttonCreate center" data-inline="true"><ArrowBackIcon /> Back </button></Link>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Dashboard;