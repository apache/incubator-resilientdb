/*global chrome*/
import './css/App.css';
import React, { useContext } from 'react';
import Home from "./pages/Home";
import SignUp from "./pages/SignUp";
import Login from "./pages/Login";
import Dashboard from "./pages/Dashboard";
import Logs from "./pages/Logs";
import { Routes, Route, Navigate } from 'react-router-dom';
import { GlobalContext } from './context/GlobalContext';

function App() {
  const { isAuthenticated } = useContext(GlobalContext);

  return (
    <Routes>
      {!isAuthenticated ? (
        <>
          <Route path="/" element={<Home />} />
          <Route path="/signup" element={<SignUp />} />
          <Route path="/login" element={<Login />} />
          <Route path="*" element={<Navigate to="/" replace />} />
        </>
      ) : (
        <>
          <Route path="/dashboard" element={<Dashboard />} />
          <Route path="*" element={<Navigate to="/dashboard" replace />} />
        </>
      )}
    </Routes>
  );
}

export default App;