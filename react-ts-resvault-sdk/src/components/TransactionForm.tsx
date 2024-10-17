// src/components/TransactionForm.tsx
import React, { useState, useEffect, useRef } from 'react';
import ResVaultSDK from 'resvault-sdk';
import '../App.css';
import NotificationModal from './NotificationModal';

interface TransactionFormProps {
  onLogout: () => void;
  token: string | null;
}

const TransactionForm: React.FC<TransactionFormProps> = ({ onLogout, token }) => {
  const [amount, setAmount] = useState<string>('');
  const [data, setData] = useState<string>('');
  const [recipient, setRecipient] = useState<string>('');
  const [showModal, setShowModal] = useState<boolean>(false);
  const [modalTitle, setModalTitle] = useState<string>('');
  const [modalMessage, setModalMessage] = useState<string>('');

  const sdkRef = useRef<ResVaultSDK | null>(null);

  if (!sdkRef.current) {
    sdkRef.current = new ResVaultSDK();
  }

  useEffect(() => {
    const sdk = sdkRef.current;
    if (!sdk) return;

    const messageHandler = (event: MessageEvent) => {
      const message = event.data;
      console.log('Received message:', message);

      if (message && message.type === 'FROM_CONTENT_SCRIPT' && message.data && message.data.success !== undefined) {
        if (message.data.success) {
          console.log('Transaction submitted successfully:', message.data);
          setModalTitle('Success');
          setModalMessage('Transaction successful! ID: ' + message.data.data.postTransaction.id);
        } else {
          console.error('Transaction submission failed:', message.data.error || message.data.errors);
          setModalTitle('Transaction Failed');
          setModalMessage('Transaction failed: ' + (message.data.error || JSON.stringify(message.data.errors)));
        }
        setShowModal(true);
      }
    };

    sdk.addMessageListener(messageHandler);

    return () => {
      sdk.removeMessageListener(messageHandler);
    };
  }, []);

  const handleSubmit = (e: React.FormEvent) => {
    e.preventDefault();

    if (!recipient) {
      setModalTitle('Validation Error');
      setModalMessage('Please enter a recipient address.');
      setShowModal(true);
      return;
    }

    let parsedData = {};
    if (data.trim() !== '') {
      try {
        parsedData = JSON.parse(data);
      } catch (error) {
        setModalTitle('Validation Error');
        setModalMessage('Invalid JSON format in the data field. Please check and try again.');
        setShowModal(true);
        return;
      }
    }

    if (sdkRef.current) {
      sdkRef.current.sendMessage({
        type: 'commit',
        direction: 'commit',
        amount: amount,
        data: parsedData,
        recipient: recipient,
      });
    } else {
      console.error('SDK is not initialized');
      setModalTitle('Error');
      setModalMessage('SDK is not initialized.');
      setShowModal(true);
    }
  };

  const handleLogout = () => {
    onLogout();
  };

  const handleCloseModal = () => setShowModal(false);

  return (
    <>
      <div className="page-container">
        <div className="form-container">
          {/* Header with Logout Button */}
          <div className="d-flex justify-content-between align-items-center mb-4">
            <h2 className="heading">Submit Transaction</h2>
            <button type="button" className="btn btn-danger logout-button" onClick={handleLogout}>
              Logout
            </button>
          </div>

          {/* Transaction Form */}
          <form onSubmit={handleSubmit}>
            {/* Input for Amount */}
            <div className="form-group mb-3">
              <input
                type="text"
                className="form-control"
                placeholder="Enter your amount here"
                value={amount}
                onChange={(e) => setAmount(e.target.value)}
              />
            </div>

            {/* Input for Data */}
            <div className="form-group mb-3">
              <input
                type="text"
                className="form-control"
                placeholder="Enter your data here (JSON)"
                value={data}
                onChange={(e) => setData(e.target.value)}
              />
            </div>

            {/* Input for Recipient Address */}
            <div className="form-group mb-4">
              <input
                type="text"
                className="form-control"
                placeholder="Enter recipient address here"
                value={recipient}
                onChange={(e) => setRecipient(e.target.value)}
              />
            </div>

            {/* Submit Button */}
            <div className="form-group text-center">
              <button type="submit" className="btn btn-primary button">
                Submit Transaction
              </button>
            </div>
          </form>
        </div>
      </div>

      {/* Notification Modal */}
      <NotificationModal
        show={showModal}
        title={modalTitle}
        message={modalMessage}
        onClose={handleCloseModal}
      />
    </>
  );
};

export default TransactionForm;