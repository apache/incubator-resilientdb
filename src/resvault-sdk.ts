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
  public sendMessage(message: Message): void {
    window.postMessage(message, this.targetOrigin);
  }

  /**
   * Commit transaction.
   * @param {Object} params - Parameters for the transaction.
   * @param {string} params.amount - The amount.
   * @param {any} params.data - The data.
   * @param {string} params.recipient - The recipient.
   * @param {Object} [params.styles] - Optional styles to override the modal's appearance.
   */
  public commitTransaction({
    amount,
    data,
    recipient,
    styles = {},
  }: {
    amount: string;
    data: any;
    recipient: string;
    styles?: any;
  }) {
    this.sendMessage({
      type: 'commit',
      direction: 'commit',
      amount,
      data,
      recipient,
      styles,
    });
  }

  /**
   * Custom transaction.
   * @param {Object} params - Parameters for the transaction.
   * @param {any} params.data - The data.
   * @param {string} params.recipient - The recipient.
   * @param {string} [params.customMessage] - Custom message for the modal.
   * @param {Object} [params.styles] - Optional styles to override the modal's appearance.
   */
  public customTransaction({
    data,
    recipient,
    customMessage,
    styles = {},
  }: {
    data: any;
    recipient: string;
    customMessage?: string;
    styles?: any;
  }) {
    this.sendMessage({
      type: 'custom',
      direction: 'custom',
      data,
      recipient,
      customMessage,
      styles,
    });
  }

  /**
   * Add a message listener for the content script.
   * @param {Function} handler - The handler to invoke when a message is received.
   */
  public addMessageListener(handler: (event: MessageEvent) => void): void {
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
  public removeMessageListener(handler: (event: MessageEvent) => void): void {
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
