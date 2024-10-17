// src/App.tsx
import React, { useState, useEffect } from 'react';
import 'bootstrap/dist/css/bootstrap.min.css';
import './App.css';
import Login from './components/Login';
import TransactionForm from './components/TransactionForm';
import Loader from './components/Loader'; // Ensure Loader is imported

function App() {
  const [isAuthenticated, setIsAuthenticated] = useState<boolean>(false);
  const [token, setToken] = useState<string | null>(null);
  const [isLoadingAfterLogin, setIsLoadingAfterLogin] = useState<boolean>(false);

  useEffect(() => {
    // Check session storage for token on mount
    const storedToken = sessionStorage.getItem('token');
    if (storedToken) {
      setToken(storedToken);
      setIsAuthenticated(true);
    }
  }, []);

  const handleLogin = (authToken: string) => {
    setIsLoadingAfterLogin(true); // Show loader
    setToken(authToken);
    sessionStorage.setItem('token', authToken);

    // After 2 seconds, show TransactionForm
    setTimeout(() => {
      setIsAuthenticated(true);
      setIsLoadingAfterLogin(false);
    }, 2000);
  };

  const handleLogout = () => {
    setIsAuthenticated(false);
    setToken(null);
    sessionStorage.removeItem('token');
  };

  return (
    <div className="App">
      {isLoadingAfterLogin && <Loader />}
      {!isLoadingAfterLogin && isAuthenticated ? (
        <TransactionForm onLogout={handleLogout} token={token} />
      ) : (
        <Login onLogin={handleLogin} />
      )}
    </div>
  );
}

export default App;