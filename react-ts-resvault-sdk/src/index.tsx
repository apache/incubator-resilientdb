// src/index.tsx
import React from 'react';
import ReactDOM from 'react-dom';
import App from './App';
import 'bootstrap/dist/css/bootstrap.min.css';

// Polyfill Buffer and process
import { Buffer } from 'buffer';
import process from 'process';

(window as any).Buffer = Buffer;
(window as any).process = process;

ReactDOM.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>,
  document.getElementById('root')
);