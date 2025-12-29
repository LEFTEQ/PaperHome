import { useEffect, useState, useCallback, useRef } from 'react';
import { useWebSocketMessage } from './use-websocket';
import type {
  WebSocketMessage,
  TelemetryPayload,
  StatusPayload,
  HueRoomPayload,
  TadoRoomPayload,
} from '@/lib/websocket';

// ─────────────────────────────────────────────────────────────────────────────
// Types
// ─────────────────────────────────────────────────────────────────────────────

export interface DeviceState {
  id: string;
  isOnline: boolean;
  lastSeen?: Date;
  telemetry?: {
    co2?: number;
    temperature?: number;
    humidity?: number;
    battery?: number;
    updatedAt: Date;
  };
  hueRooms?: HueRoomPayload[];
  tadoRooms?: TadoRoomPayload[];
}

// ─────────────────────────────────────────────────────────────────────────────
// useDeviceSubscription - Subscribe to all updates for a specific device
// ─────────────────────────────────────────────────────────────────────────────

export interface UseDeviceSubscriptionOptions {
  /** Device ID to subscribe to */
  deviceId: string;
  /** Initial device state */
  initialState?: Partial<DeviceState>;
  /** Callback when telemetry updates */
  onTelemetry?: (data: TelemetryPayload) => void;
  /** Callback when status changes */
  onStatusChange?: (online: boolean) => void;
  /** Callback when Hue state updates */
  onHueUpdate?: (rooms: HueRoomPayload[]) => void;
  /** Callback when Tado state updates */
  onTadoUpdate?: (rooms: TadoRoomPayload[]) => void;
}

export function useDeviceSubscription({
  deviceId,
  initialState,
  onTelemetry,
  onStatusChange,
  onHueUpdate,
  onTadoUpdate,
}: UseDeviceSubscriptionOptions): DeviceState {
  const [state, setState] = useState<DeviceState>({
    id: deviceId,
    isOnline: initialState?.isOnline ?? false,
    lastSeen: initialState?.lastSeen,
    telemetry: initialState?.telemetry,
    hueRooms: initialState?.hueRooms,
    tadoRooms: initialState?.tadoRooms,
  });

  // Keep callback refs up to date
  const onTelemetryRef = useRef(onTelemetry);
  const onStatusChangeRef = useRef(onStatusChange);
  const onHueUpdateRef = useRef(onHueUpdate);
  const onTadoUpdateRef = useRef(onTadoUpdate);

  useEffect(() => {
    onTelemetryRef.current = onTelemetry;
    onStatusChangeRef.current = onStatusChange;
    onHueUpdateRef.current = onHueUpdate;
    onTadoUpdateRef.current = onTadoUpdate;
  }, [onTelemetry, onStatusChange, onHueUpdate, onTadoUpdate]);

  // Handle telemetry updates
  useWebSocketMessage<TelemetryPayload>('telemetry', (message) => {
    if (message.deviceId !== deviceId) return;

    setState((prev) => ({
      ...prev,
      telemetry: {
        ...prev.telemetry,
        ...message.payload,
        updatedAt: new Date(),
      },
    }));

    onTelemetryRef.current?.(message.payload);
  });

  // Handle status updates
  useWebSocketMessage<StatusPayload>('status', (message) => {
    if (message.deviceId !== deviceId) return;

    const wasOnline = state.isOnline;
    const isNowOnline = message.payload.online;

    setState((prev) => ({
      ...prev,
      isOnline: isNowOnline,
      lastSeen: new Date(message.payload.lastSeen),
    }));

    if (wasOnline !== isNowOnline) {
      onStatusChangeRef.current?.(isNowOnline);
    }
  });

  // Handle Hue state updates
  useWebSocketMessage<HueRoomPayload[]>('hue_state', (message) => {
    if (message.deviceId !== deviceId) return;

    setState((prev) => ({
      ...prev,
      hueRooms: message.payload,
    }));

    onHueUpdateRef.current?.(message.payload);
  });

  // Handle Tado state updates
  useWebSocketMessage<TadoRoomPayload[]>('tado_state', (message) => {
    if (message.deviceId !== deviceId) return;

    setState((prev) => ({
      ...prev,
      tadoRooms: message.payload,
    }));

    onTadoUpdateRef.current?.(message.payload);
  });

  return state;
}

// ─────────────────────────────────────────────────────────────────────────────
// useDevicesSubscription - Subscribe to updates for all devices
// ─────────────────────────────────────────────────────────────────────────────

export function useDevicesSubscription(): Map<string, DeviceState> {
  const [devices, setDevices] = useState<Map<string, DeviceState>>(new Map());

  const updateDevice = useCallback(
    (deviceId: string, updates: Partial<DeviceState>) => {
      setDevices((prev) => {
        const newMap = new Map(prev);
        const existing = newMap.get(deviceId) || {
          id: deviceId,
          isOnline: false,
        };
        newMap.set(deviceId, { ...existing, ...updates });
        return newMap;
      });
    },
    []
  );

  // Handle telemetry updates
  useWebSocketMessage<TelemetryPayload>('telemetry', (message) => {
    if (!message.deviceId) return;

    updateDevice(message.deviceId, {
      telemetry: {
        ...message.payload,
        updatedAt: new Date(),
      },
    });
  });

  // Handle status updates
  useWebSocketMessage<StatusPayload>('status', (message) => {
    if (!message.deviceId) return;

    updateDevice(message.deviceId, {
      isOnline: message.payload.online,
      lastSeen: new Date(message.payload.lastSeen),
    });
  });

  // Handle Hue state updates
  useWebSocketMessage<HueRoomPayload[]>('hue_state', (message) => {
    if (!message.deviceId) return;

    updateDevice(message.deviceId, {
      hueRooms: message.payload,
    });
  });

  // Handle Tado state updates
  useWebSocketMessage<TadoRoomPayload[]>('tado_state', (message) => {
    if (!message.deviceId) return;

    updateDevice(message.deviceId, {
      tadoRooms: message.payload,
    });
  });

  return devices;
}

// ─────────────────────────────────────────────────────────────────────────────
// useTelemetryHistory - Track telemetry history for charts
// ─────────────────────────────────────────────────────────────────────────────

export interface TelemetryPoint {
  timestamp: Date;
  co2?: number;
  temperature?: number;
  humidity?: number;
  battery?: number;
}

export function useTelemetryHistory(
  deviceId: string,
  maxPoints = 24
): TelemetryPoint[] {
  const [history, setHistory] = useState<TelemetryPoint[]>([]);

  useWebSocketMessage<TelemetryPayload>('telemetry', (message) => {
    if (message.deviceId !== deviceId) return;

    setHistory((prev) => {
      const newPoint: TelemetryPoint = {
        timestamp: new Date(),
        ...message.payload,
      };

      const updated = [...prev, newPoint];

      // Keep only the last N points
      if (updated.length > maxPoints) {
        return updated.slice(-maxPoints);
      }

      return updated;
    });
  });

  return history;
}

export default useDeviceSubscription;
