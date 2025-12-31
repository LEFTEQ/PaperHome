/**
 * WebSocket Client for PaperHome real-time updates
 * Uses Socket.io to connect to NestJS WebSocket gateway
 */

import { io, Socket } from 'socket.io-client';
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
  // STCC4 sensor data
  co2?: number;
  temperature?: number;
  humidity?: number;
  battery?: number;
  // BME688/BSEC2 sensor data
  iaq?: number;
  iaqAccuracy?: number;
  pressure?: number;
  bme688Temperature?: number;
  bme688Humidity?: number;
}

export interface StatusPayload {
  online: boolean;
  lastSeen: string;
}

export interface HueRoomPayload {
  roomId: string;
  roomName: string;
  isOn: boolean;
  brightness: number;
}

export interface TadoRoomPayload {
  roomId: string;
  roomName: string;
  currentTemp: number;
  targetTemp: number;
  humidity?: number;
  isHeating: boolean;
}

export interface NotificationPayload {
  id: string;
  type: 'info' | 'warning' | 'error' | 'success';
  title: string;
  message?: string;
  deviceId?: string;
}

export type MessageHandler<T = unknown> = (message: WebSocketMessage<T>) => void;

// ─────────────────────────────────────────────────────────────────────────────
// WebSocket Client
// ─────────────────────────────────────────────────────────────────────────────

export class PaperHomeWebSocket {
  private socket: Socket | null = null;
  private url: string;
  private handlers: Map<WebSocketMessageType | '*', Set<MessageHandler>> =
    new Map();
  private connectionPromise: Promise<void> | null = null;

  constructor(url?: string) {
    // Default to current host
    const baseUrl = url || import.meta.env.VITE_API_URL || window.location.origin;
    this.url = baseUrl;
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Connection Management
  // ─────────────────────────────────────────────────────────────────────────

  async connect(): Promise<void> {
    if (this.socket?.connected) {
      return;
    }

    if (this.connectionPromise) {
      return this.connectionPromise;
    }

    this.connectionPromise = new Promise((resolve, reject) => {
      const token = getAccessToken();

      this.socket = io(this.url, {
        path: '/ws',
        transports: ['websocket', 'polling'],
        query: { token: token || '' },
        autoConnect: true,
        reconnection: true,
        reconnectionAttempts: 5,
        reconnectionDelay: 1000,
        reconnectionDelayMax: 5000,
      });

      this.socket.on('connect', () => {
        console.log('[WebSocket] Connected');
        this.connectionPromise = null;
        resolve();
      });

      this.socket.on('disconnect', (reason) => {
        console.log('[WebSocket] Disconnected:', reason);
      });

      this.socket.on('connect_error', (error) => {
        console.error('[WebSocket] Connection error:', error);
        this.connectionPromise = null;
        reject(error);
      });

      // Set up message handlers for each event type
      const eventTypes: WebSocketMessageType[] = [
        'telemetry',
        'status',
        'hue_state',
        'tado_state',
        'notification',
        'command_ack',
      ];

      for (const eventType of eventTypes) {
        this.socket.on(eventType, (data: WebSocketMessage) => {
          this.handleMessage(eventType, data);
        });
      }
    });

    return this.connectionPromise;
  }

  disconnect(): void {
    if (this.socket) {
      this.socket.disconnect();
      this.socket = null;
    }
    this.connectionPromise = null;
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Message Handling
  // ─────────────────────────────────────────────────────────────────────────

  private handleMessage(eventType: WebSocketMessageType, message: WebSocketMessage): void {
    // Call type-specific handlers
    const typeHandlers = this.handlers.get(eventType);
    if (typeHandlers) {
      typeHandlers.forEach((handler) => handler(message));
    }

    // Call wildcard handlers
    const wildcardHandlers = this.handlers.get('*');
    if (wildcardHandlers) {
      wildcardHandlers.forEach((handler) => handler(message));
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
    if (!this.socket?.connected) {
      console.warn('[WebSocket] Cannot send - not connected');
      return;
    }

    this.socket.emit(type, {
      payload,
      timestamp: new Date().toISOString(),
    });
  }

  // ─────────────────────────────────────────────────────────────────────────
  // Status
  // ─────────────────────────────────────────────────────────────────────────

  get isConnected(): boolean {
    return this.socket?.connected ?? false;
  }

  get readyState(): number {
    if (!this.socket) return WebSocket.CLOSED;
    return this.socket.connected ? WebSocket.OPEN : WebSocket.CLOSED;
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// Singleton Instance
// ─────────────────────────────────────────────────────────────────────────────

let wsInstance: PaperHomeWebSocket | null = null;

export function getWebSocket(): PaperHomeWebSocket {
  if (!wsInstance) {
    const wsUrl = import.meta.env.VITE_API_URL;
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
