/**
 * Hook for real-time updates via WebSocket
 * Automatically updates React Query cache when data arrives via WebSocket
 */

import { useEffect } from 'react';
import { useQueryClient } from '@tanstack/react-query';
import { useWebSocket, useWebSocketMessage } from './use-websocket';
import { queryKeys } from './use-devices';
import type { Device, LatestTelemetry, HueRoom, TadoRoom } from '@/lib/api';
import type {
  WebSocketMessage,
  TelemetryPayload,
  StatusPayload,
  HueRoomPayload,
  TadoRoomPayload,
} from '@/lib/websocket';

// ─────────────────────────────────────────────────────────────────────────────
// useRealtimeUpdates - Main hook that handles all real-time updates
// ─────────────────────────────────────────────────────────────────────────────

export function useRealtimeUpdates() {
  const queryClient = useQueryClient();
  const { isConnected, connect } = useWebSocket({ autoConnect: true });

  // Ensure connection on mount
  useEffect(() => {
    if (!isConnected) {
      connect().catch(console.error);
    }
  }, [isConnected, connect]);

  // Handle telemetry updates
  useWebSocketMessage<TelemetryPayload>('telemetry', (message) => {
    const { deviceId, payload, timestamp } = message;
    if (!deviceId) return;

    // Update latest telemetry cache
    queryClient.setQueryData<LatestTelemetry[]>(queryKeys.telemetryLatest, (old) => {
      if (!old) return old;

      const existing = old.find((t) => t.deviceId === deviceId);
      if (existing) {
        return old.map((t) =>
          t.deviceId === deviceId
            ? {
                ...t,
                co2: payload.co2 ?? t.co2,
                temperature: payload.temperature ?? t.temperature,
                humidity: payload.humidity ?? t.humidity,
                battery: payload.battery ?? t.battery,
                time: timestamp,
              }
            : t
        );
      } else {
        return [
          ...old,
          {
            deviceId,
            co2: payload.co2 ?? null,
            temperature: payload.temperature ?? null,
            humidity: payload.humidity ?? null,
            battery: payload.battery ?? null,
            time: timestamp,
          },
        ];
      }
    });
  });

  // Handle status updates
  useWebSocketMessage<StatusPayload>('status', (message) => {
    const { deviceId, payload } = message;
    if (!deviceId) return;

    // Update devices cache
    queryClient.setQueryData<Device[]>(queryKeys.devices, (old) => {
      if (!old) return old;

      return old.map((device) =>
        device.deviceId === deviceId
          ? {
              ...device,
              isOnline: payload.online,
              lastSeenAt: payload.lastSeen,
            }
          : device
      );
    });

    // Also update single device cache if it exists
    queryClient.setQueriesData<Device>(
      { queryKey: ['devices'], exact: false },
      (old) => {
        if (!old || old.deviceId !== deviceId) return old;
        return {
          ...old,
          isOnline: payload.online,
          lastSeenAt: payload.lastSeen,
        };
      }
    );
  });

  // Handle Hue state updates
  useWebSocketMessage<HueRoomPayload[]>('hue_state', (message) => {
    const { deviceId, payload, timestamp } = message;
    if (!deviceId || !Array.isArray(payload)) return;

    // Update Hue rooms cache
    queryClient.setQueryData<HueRoom[]>(queryKeys.hueRooms(deviceId), (old) => {
      if (!old) {
        // If no cache exists, create new entries
        return payload.map((room) => ({
          id: `${deviceId}-${room.roomId}`,
          roomId: room.roomId,
          roomName: room.roomName,
          isOn: room.isOn,
          brightness: room.brightness,
          time: timestamp,
        }));
      }

      // Update existing rooms
      return payload.map((room) => {
        const existing = old.find((r) => r.roomId === room.roomId);
        return {
          id: existing?.id ?? `${deviceId}-${room.roomId}`,
          roomId: room.roomId,
          roomName: room.roomName,
          isOn: room.isOn,
          brightness: room.brightness,
          time: timestamp,
        };
      });
    });
  });

  // Handle Tado state updates
  useWebSocketMessage<TadoRoomPayload[]>('tado_state', (message) => {
    const { deviceId, payload, timestamp } = message;
    if (!deviceId || !Array.isArray(payload)) return;

    // Update Tado rooms cache
    queryClient.setQueryData<TadoRoom[]>(queryKeys.tadoRooms(deviceId), (old) => {
      if (!old) {
        // If no cache exists, create new entries
        return payload.map((room) => ({
          id: `${deviceId}-${room.roomId}`,
          roomId: room.roomId,
          roomName: room.roomName,
          currentTemp: room.currentTemp,
          targetTemp: room.targetTemp,
          humidity: room.humidity ?? 0,
          isHeating: room.isHeating,
          heatingPower: 0,
          time: timestamp,
        }));
      }

      // Update existing rooms
      return payload.map((room) => {
        const existing = old.find((r) => r.roomId === room.roomId);
        return {
          id: existing?.id ?? `${deviceId}-${room.roomId}`,
          roomId: room.roomId,
          roomName: room.roomName,
          currentTemp: room.currentTemp,
          targetTemp: room.targetTemp,
          humidity: room.humidity ?? existing?.humidity ?? 0,
          isHeating: room.isHeating,
          heatingPower: existing?.heatingPower ?? 0,
          time: timestamp,
        };
      });
    });
  });

  return { isConnected };
}

// ─────────────────────────────────────────────────────────────────────────────
// useDeviceSubscription - Subscribe to updates for a specific device
// ─────────────────────────────────────────────────────────────────────────────

export function useDeviceSubscription(deviceId: string | undefined) {
  const queryClient = useQueryClient();

  // Handle telemetry for specific device
  useWebSocketMessage<TelemetryPayload>('telemetry', (message) => {
    if (!deviceId || message.deviceId !== deviceId) return;

    // Invalidate telemetry history to trigger refetch
    queryClient.invalidateQueries({
      queryKey: queryKeys.telemetryHistory(deviceId),
    });
  });
}
