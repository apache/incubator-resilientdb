/*global chrome*/
import './css/App.css';
import React, { useContext } from 'react';
import Home from "./pages/Home";
import SignUp from "./pages/SignUp";
import Login from "./pages/Login";
import Dashboard from "./pages/Dashboard";
import Logs from "./pages/Logs";
import Contract from "./pages/Contract";
import { Routes, Route, Navigate } from 'react-router-dom';
import { GlobalContext } from './context/GlobalContext';

function App() {
  const { serviceMode, isAuthenticated } = useContext(GlobalContext);

  return (
    <Routes>
      {!isAuthenticated ? (
        <>
          <Route path="/" element={<Home />} />
          <Route path="/signup" element={<SignUp />} />
          <Route path="/login" element={<Login />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </>
      ) : serviceMode === 'KV' ? (
        <>
          <Route path="/dashboard" element={<Dashboard />} />
          <Route path="*" element={<Navigate to="/dashboard" replace />} />
        </>
      ) : (
        <>
          <Route path="/contract" element={<Contract />} />
          <Route path="*" element={<Navigate to="/contract" replace />} />
        </>
      )}
    </Routes>
  );
}

export default App;