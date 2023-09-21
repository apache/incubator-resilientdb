import './css/App.css';
import React, { useState } from 'react';
import Home from "./pages/Home";
import SignUp from "./pages/SignUp";
import Login from "./pages/Login";
import Dashboard from "./pages/Dashboard";
import Transaction from './pages/Transaction';
import Logs from "./pages/Logs";
import { Navigate, Route, Routes } from "react-router-dom";
import { useNavigate } from "react-router-dom";

function App() {
  const navigate = useNavigate();
  const [values, setValues] = useState({
    password: "",
    showPassword: false,
  })
  const [loginValues, setLoginValues] = useState({
    password: "",
    showPassword: false,
  })
  const [confirmValues, setConfirmValues] = useState({
    password: "",
    showPassword: false,
  })

  const props = {
    navigate,
    values,
    loginValues,
    confirmValues
  }

  const propsChange = {
    setValues,
    setLoginValues,
    setConfirmValues
  }

  return (
    <Routes>
      <Route path='/' element={<Home {...props} {...propsChange} />} />
      <Route path='/signup' element={<SignUp {...props} {...propsChange} />} />
      <Route path='/login' element={<Login {...props} {...propsChange} />} />
      <Route path='/dashboard' element={<Dashboard {...props} {...propsChange} />} />
      <Route path='/transaction' element={<Transaction {...props} {...propsChange} />} />
      <Route path='/logs' element={<Logs {...props} {...propsChange} />} />
      <Route path='*' element={<Navigate to='/' />} />
    </Routes>
  )
}

export default App;
