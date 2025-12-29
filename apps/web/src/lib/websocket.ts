/**
 * WebSocket Client for PaperHome real-time updates
 *
 * Handles connection management, reconnection, and message routing
 */

import { getAccessToken } from './api/client';

// ─────────────────────────────────────────────────────────────────────────────
// Types
// ─────────────────────────────────────────────────────────────────────────────

export type WebSocketMessageType =
  | 'telemetry'
  | 'status'
  | 'hue_state'
  | 'tado_state'
  | 'notification'
  | 'command_ack';

export interface WebSocketMessage<T = unknown> {
  type: WebSocketMessageType;
  deviceId?: string;
  payload: T;
  timestamp: string;
}

export interface TelemetryPayload {
  co2?: number;
  temperature?: number;
  humidity?: number;
  battery?: number;
}

export interface StatusPayload {
  online: boolean;
  lastSeen: string;
}

export interface HueRoomPayload {
  id: string;
  name: string;
  isOn: boolean;
  brightness: number;
}

export interface TadoRoomPayload {
  id: string;
  name: string;
  currentTemp: number;
  targetTemp: number;
  humidity: number;
  isHeating: boolean;
  mode: 'heat' | 'cool' | 'auto' | 'off';
}

export interface NotificationPayload {
  id: string;
  type: 'info' | 'warning' | 'error' | 'success';
  title: string;
  message: string;
  deviceId?: string;
  read: boolean;
  createdAt: string;
}

export type MessageHandler<T = unknown> = (message: WebSocketMessage<T>) => void;

// ─────────────────────────────────────────────────────────────────────────────
// WebSocket Client
// ─────────────────────────────────────────────────────────────────────────────

export class PaperHomeWebSocket {
  private ws: WebSocket | null = null;
  private url: string;
  private reconnectAttempts = 0;
  private maxReconnectAttempts = 5;
  private reconnectDelay = 1000;
  private handlers: Map<WebSocketMessageType | '*', Set<MessageHandler>> =
    new Map();
  private connectionPromise: Promise<void> | null = null;
  private isConnecting = false;

  constructor(url?: string) {
    // Default to current host with ws/wss protocol
    const protocol = window.location.protocol === 'https:' ? 'wss:' : 'ws:';
    this.url = url || `${protocol}//${window.location.host}/ws`;
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Connection Management
  // ─────────────────────────────────────────────────────────────────────────

  async connect(): Promise<void> {
    if (this.ws?.readyState === WebSocket.OPEN) {
      return;
    }

    if (this.isConnecting && this.connectionPromise) {
      return this.connectionPromise;
    }

    this.isConnecting = true;
    this.connectionPromise = new Promise((resolve, reject) => {
      try {
        const token = getAccessToken();
        const urlWithAuth = token ? `${this.url}?token=${token}` : this.url;

        this.ws = new WebSocket(urlWithAuth);

        this.ws.onopen = () => {
          console.log('[WebSocket] Connected');
          this.reconnectAttempts = 0;
          this.isConnecting = false;
          resolve();
        };

        this.ws.onclose = (event) => {
          console.log('[WebSocket] Disconnected:', event.code, event.reason);
          this.isConnecting = false;
          this.handleReconnect();
        };

        this.ws.onerror = (error) => {
          console.error('[WebSocket] Error:', error);
          this.isConnecting = false;
          reject(error);
        };

        this.ws.onmessage = (event) => {
          this.handleMessage(event.data);
        };
      } catch (error) {
        this.isConnecting = false;
        reject(error);
      }
    });

    return this.connectionPromise;
  }

  disconnect(): void {
    if (this.ws) {
      this.ws.close(1000, 'Client disconnect');
      this.ws = null;
    }
    this.reconnectAttempts = this.maxReconnectAttempts; // Prevent reconnection
  }

  private handleReconnect(): void {
    if (this.reconnectAttempts >= this.maxReconnectAttempts) {
      console.log('[WebSocket] Max reconnect attempts reached');
      return;
    }

    this.reconnectAttempts++;
    const delay = this.reconnectDelay * Math.pow(2, this.reconnectAttempts - 1);

    console.log(
      `[WebSocket] Reconnecting in ${delay}ms (attempt ${this.reconnectAttempts})`
    );

    setTimeout(() => {
      this.connect().catch((error) => {
        console.error('[WebSocket] Reconnection failed:', error);
      });
    }, delay);
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Message Handling
  // ─────────────────────────────────────────────────────────────────────────

  private handleMessage(data: string): void {
    try {
      const message: WebSocketMessage = JSON.parse(data);

      // Call type-specific handlers
      const typeHandlers = this.handlers.get(message.type);
      if (typeHandlers) {
        typeHandlers.forEach((handler) => handler(message));
      }

      // Call wildcard handlers
      const wildcardHandlers = this.handlers.get('*');
      if (wildcardHandlers) {
        wildcardHandlers.forEach((handler) => handler(message));
      }
    } catch (error) {
      console.error('[WebSocket] Failed to parse message:', error);
    }
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Subscriptions
  // ─────────────────────────────────────────────────────────────────────────

  subscribe<T = unknown>(
    type: WebSocketMessageType | '*',
    handler: MessageHandler<T>
  ): () => void {
    if (!this.handlers.has(type)) {
      this.handlers.set(type, new Set());
    }

    const handlers = this.handlers.get(type)!;
    handlers.add(handler as MessageHandler);

    // Return unsubscribe function
    return () => {
      handlers.delete(handler as MessageHandler);
      if (handlers.size === 0) {
        this.handlers.delete(type);
      }
    };
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Sending Messages
  // ─────────────────────────────────────────────────────────────────────────

  send(type: string, payload: unknown): void {
    if (this.ws?.readyState !== WebSocket.OPEN) {
      console.warn('[WebSocket] Cannot send - not connected');
      return;
    }

    const message = {
      type,
      payload,
      timestamp: new Date().toISOString(),
    };

    this.ws.send(JSON.stringify(message));
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Status
  // ─────────────────────────────────────────────────────────────────────────

  get isConnected(): boolean {
    return this.ws?.readyState === WebSocket.OPEN;
  }

  get readyState(): number {
    return this.ws?.readyState ?? WebSocket.CLOSED;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Singleton Instance
// ─────────────────────────────────────────────────────────────────────────────

let wsInstance: PaperHomeWebSocket | null = null;

export function getWebSocket(): PaperHomeWebSocket {
  if (!wsInstance) {
    // Use environment variable or default
    const wsUrl = import.meta.env.VITE_WS_URL;
    wsInstance = new PaperHomeWebSocket(wsUrl);
  }
  return wsInstance;
}

export function resetWebSocket(): void {
  if (wsInstance) {
    wsInstance.disconnect();
    wsInstance = null;
  }
}
