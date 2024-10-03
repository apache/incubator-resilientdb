export interface Message {
  type: string;
  [key: string]: any;
}

export default class ResVaultSDK {
  private targetOrigin: string;
  private messageHandlers: Array<(event: MessageEvent) => void>;
  private _messageHandler: (event: MessageEvent) => void;

  constructor(targetOrigin: string = '*') {
    this.targetOrigin = targetOrigin;
    this.messageHandlers = [];
    this._messageHandler = this._handleMessage.bind(this);
  }

  /**
   * Send a message using postMessage.
   * @param {Message} message - The message to send.
   */
  sendMessage(message: Message): void {
    window.postMessage(message, this.targetOrigin);
  }

  /**
   * Add a message listener for the content script.
   * @param {Function} handler - The handler to invoke when a message is received.
   */
  addMessageListener(handler: (event: MessageEvent) => void): void {
    if (typeof handler !== 'function') {
      throw new Error('Handler must be a function');
    }
    this.messageHandlers.push(handler);
    if (this.messageHandlers.length === 1) {
      this._startListening();
    }
  }

  /**
   * Remove a message listener.
   * @param {Function} handler - The handler to remove.
   */
  removeMessageListener(handler: (event: MessageEvent) => void): void {
    const index = this.messageHandlers.indexOf(handler);
    if (index !== -1) {
      this.messageHandlers.splice(index, 1);
    }
    if (this.messageHandlers.length === 0) {
      this._stopListening();
    }
  }

  private _startListening(): void {
    window.addEventListener('message', this._messageHandler);
  }

  private _stopListening(): void {
    window.removeEventListener('message', this._messageHandler);
  }

  private _handleMessage(event: MessageEvent): void {
    if (
      event.source === window &&
      event.data &&
      event.data.type === 'FROM_CONTENT_SCRIPT'
    ) {
      for (const handler of this.messageHandlers) {
        handler(event);
      }
    }
  }
}
