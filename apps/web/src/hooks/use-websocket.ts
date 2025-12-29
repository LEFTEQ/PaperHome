import { useEffect, useState, useCallback, useRef } from 'react';
import {
  getWebSocket,
  type WebSocketMessage,
  type WebSocketMessageType,
  type MessageHandler,
} from '@/lib/websocket';

// ─────────────────────────────────────────────────────────────────────────────
// useWebSocket - Base WebSocket connection hook
// ─────────────────────────────────────────────────────────────────────────────

export interface UseWebSocketOptions {
  /** Auto-connect on mount (default: true) */
  autoConnect?: boolean;
  /** Callback when connection opens */
  onConnect?: () => void;
  /** Callback when connection closes */
  onDisconnect?: () => void;
  /** Callback when error occurs */
  onError?: (error: Event) => void;
}

export interface UseWebSocketReturn {
  /** Whether WebSocket is connected */
  isConnected: boolean;
  /** Current connection state */
  readyState: number;
  /** Connect to WebSocket server */
  connect: () => Promise<void>;
  /** Disconnect from WebSocket server */
  disconnect: () => void;
  /** Send a message */
  send: (type: string, payload: unknown) => void;
  /** Subscribe to message type */
  subscribe: <T = unknown>(
    type: WebSocketMessageType | '*',
    handler: MessageHandler<T>
  ) => () => void;
}

export function useWebSocket(
  options: UseWebSocketOptions = {}
): UseWebSocketReturn {
  const { autoConnect = true, onConnect, onDisconnect, onError } = options;
  const [isConnected, setIsConnected] = useState(false);
  const [readyState, setReadyState] = useState(WebSocket.CLOSED);
  const wsRef = useRef(getWebSocket());

  // Connection status polling
  useEffect(() => {
    const checkConnection = () => {
      const ws = wsRef.current;
      setIsConnected(ws.isConnected);
      setReadyState(ws.readyState);
    };

    // Check immediately
    checkConnection();

    // Poll for status changes
    const interval = setInterval(checkConnection, 1000);

    return () => clearInterval(interval);
  }, []);

  // Auto-connect on mount
  useEffect(() => {
    if (!autoConnect) return;

    const ws = wsRef.current;
    ws.connect()
      .then(() => {
        setIsConnected(true);
        onConnect?.();
      })
      .catch((error) => {
        console.error('[useWebSocket] Connection error:', error);
        onError?.(error);
      });

    return () => {
      // Don't disconnect on unmount - let other components share the connection
    };
  }, [autoConnect, onConnect, onError]);

  const connect = useCallback(async () => {
    const ws = wsRef.current;
    await ws.connect();
    setIsConnected(true);
    onConnect?.();
  }, [onConnect]);

  const disconnect = useCallback(() => {
    const ws = wsRef.current;
    ws.disconnect();
    setIsConnected(false);
    onDisconnect?.();
  }, [onDisconnect]);

  const send = useCallback((type: string, payload: unknown) => {
    wsRef.current.send(type, payload);
  }, []);

  const subscribe = useCallback(
    <T = unknown>(
      type: WebSocketMessageType | '*',
      handler: MessageHandler<T>
    ) => {
      return wsRef.current.subscribe(type, handler);
    },
    []
  );

  return {
    isConnected,
    readyState,
    connect,
    disconnect,
    send,
    subscribe,
  };
}

// ─────────────────────────────────────────────────────────────────────────────
// useWebSocketMessage - Subscribe to specific message type
// ─────────────────────────────────────────────────────────────────────────────

export function useWebSocketMessage<T = unknown>(
  type: WebSocketMessageType | '*',
  handler: (message: WebSocketMessage<T>) => void
): void {
  const { subscribe } = useWebSocket({ autoConnect: true });
  const handlerRef = useRef(handler);

  // Keep handler ref up to date
  useEffect(() => {
    handlerRef.current = handler;
  }, [handler]);

  // Subscribe to messages
  useEffect(() => {
    const unsubscribe = subscribe<T>(type, (message) => {
      handlerRef.current(message);
    });

    return unsubscribe;
  }, [type, subscribe]);
}

export default useWebSocket;
