import logo from '../logo.png';
import '../App.css';
import Footer from "../components/Footer";
import ArrowBackIcon from '@mui/icons-material/ArrowBack';
import { useLocation } from "react-router-dom";

function Dashboard(props) {
  const location = useLocation();
  props.setFooter("footerLogin");

  const back = async () => {
    const store = location.state;
    props.navigate("/login", {state: store});
  }
 
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
        <p className="publicKeyStyle"><b>Public Key:</b> {location.state.publicKey}</p>
      </div>
      <div className="paymentBottomKey vcenter">
        <p className="publicKeyStyle"><b>Private Key:</b> {location.state.privateKey}</p>
      </div>

      <div className="paymentBottomDashboard vcenter">
        <textarea className="scrollabletextbox" rows="3" cols="35"> Insert Dynamic Manifest file here</textarea>
      </div>

      <div className="paymentBottomDashboardBack vcenter">
        <button className="buttonCreate center" data-inline="true"> Submit </button>
        <button className="buttonCreate center" data-inline="true" onClick={back}><ArrowBackIcon /> Back </button>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Dashboard;