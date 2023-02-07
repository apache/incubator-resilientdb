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

      <div className="paymentTop vcenter">
        <p className="publicKeyStyle"><b>Public Key:</b> {props.publicKey}</p>
      </div>
      <div className="paymentBottom vcenter">
        <p className="publicKeyStyle"><b>Private Key:</b> {props.privateKey}</p>
      </div>

      <div className="paymentBottomDashboard vcenter">
      <Link to="/login" className="routing"><button className="buttonCreate center"><ArrowBackIcon /> Back</button></Link>
      </div>

      <Footer {...props} />
    </div>
  </div>
  )
}

export default Dashboard;