/*global chrome*/
import './css/App.css';
import React from 'react';
import Home from "./pages/Home";
import SignUp from "./pages/SignUp";
import Login from "./pages/Login";
import Dashboard from "./pages/Dashboard";
import Logs from "./pages/Logs";
import { Navigate, Route, Routes } from "react-router-dom";
import { GlobalProvider } from './context/GlobalContext';

function App() {
  return (
    <GlobalProvider>
      <Routes>
        <Route path='/' element={<Home />} />
        <Route path='/signup' element={<SignUp />} />
        <Route path='/login' element={<Login />} />
        <Route path='/dashboard' element={<Dashboard />} />
        <Route path='/logs' element={<Logs />} />
        <Route path='*' element={<Navigate to='/' />} />
      </Routes>
    </GlobalProvider>
  );
}

export default App;
