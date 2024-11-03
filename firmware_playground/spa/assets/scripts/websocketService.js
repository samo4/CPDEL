class WebSocketService {
  constructor(url) {
    this.url = url
    this.socket = null
    this.eventTarget = new EventTarget()
    this._isConnected = false
  }

  connect() {
    this.socket = new WebSocket(this.url)

    this.socket.onopen = () => {
      this.eventTarget.dispatchEvent(new Event('connected'))
      this._isConnected = true
    }

    this.socket.onclose = () => {
      console.log('WebSocket connection closed')
      this._isConnected = false
    }

    this.socket.onerror = (error) => {
      console.error('WebSocket error:', error)
    }

    this.socket.onmessage = message => {
      const e = new CustomEvent('message',{ detail: JSON.parse(message.data) })
      this.eventTarget.dispatchEvent(e)
    }
  }

  sendMessage(message) {
    if (this.socket && this.socket.readyState === WebSocket.OPEN) {
      this.socket.send(JSON.stringify(message))
    }
  }

  onMessage(callback) {
    this.eventTarget.addEventListener('message', callback)
  }

  onConnected(callback) {
    this.eventTarget.addEventListener('connected', callback)
  }

  close() {
    if (this.socket) {
      this.socket.close()
    }
  }

  get isConnected() {
    return this._isConnected
  }
}

  /* handle:
  socket.readyState ===  WebSocket.CONNECTING (0)// disconnected
  socket.readyState === WebSocket.CLOSING (2)
  socket.readyState === WebSocket.CLOSED (3)
  socket.readyState === WebSocket.OPEN (1) // ready
  */

const wsService = new WebSocketService('ws://192.168.88.128/dashws')
export default wsService
