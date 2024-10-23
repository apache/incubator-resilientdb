import React from 'react';
import { Spinner } from 'react-bootstrap';
import '../App.css';

const Loader = () => {
  return (
    <div className="loader-overlay">
      <Spinner animation="border" variant="primary" />
    </div>
  );
};

export default Loader;