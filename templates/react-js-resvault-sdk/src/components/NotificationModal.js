import React from 'react';
import { Modal, Button } from 'react-bootstrap';

const NotificationModal = ({ show, title, message, onClose }) => {
  return (
    <Modal show={show} onHide={onClose} centered className="custom-modal">
      <Modal.Header closeButton className="custom-modal-header">
        <Modal.Title>{title}</Modal.Title>
      </Modal.Header>
      <Modal.Body className="custom-modal-body">
        <p>{message}</p>
      </Modal.Body>
      <Modal.Footer className="custom-modal-footer">
        <Button variant="primary" onClick={onClose}>
          OK
        </Button>
      </Modal.Footer>
    </Modal>
  );
};

export default NotificationModal;