/**
 * React Query hooks for PaperHome API
 */

import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';
import {
  devicesApi,
  telemetryApi,
  hueApi,
  tadoApi,
  type Device,
  type LatestTelemetry,
  type Telemetry,
  type HueRoom,
  type TadoRoom,
  type TelemetryAggregate,
} from '@/lib/api';

// ─────────────────────────────────────────────────────────────────────────────
// Query Keys
// ─────────────────────────────────────────────────────────────────────────────

export const queryKeys = {
  devices: ['devices'] as const,
  device: (id: string) => ['devices', id] as const,
  unclaimed: ['devices', 'unclaimed'] as const,
  telemetryLatest: ['telemetry', 'latest'] as const,
  telemetryHistory: (deviceId: string) => ['telemetry', deviceId, 'history'] as const,
  telemetryAggregates: (deviceId: string) => ['telemetry', deviceId, 'aggregates'] as const,
  hueRooms: (deviceId: string) => ['hue', deviceId, 'rooms'] as const,
  tadoRooms: (deviceId: string) => ['tado', deviceId, 'rooms'] as const,
};

// ─────────────────────────────────────────────────────────────────────────────
// Device Hooks
// ─────────────────────────────────────────────────────────────────────────────

export function useDevices() {
  return useQuery<Device[]>({
    queryKey: queryKeys.devices,
    queryFn: devicesApi.getAll,
    staleTime: 30_000, // 30 seconds
    refetchOnWindowFocus: true,
  });
}

export function useDevice(id: string | undefined) {
  return useQuery<Device>({
    queryKey: queryKeys.device(id!),
    queryFn: () => devicesApi.getById(id!),
    enabled: !!id,
    staleTime: 30_000,
  });
}

export function useUnclaimedDevices() {
  return useQuery<Device[]>({
    queryKey: queryKeys.unclaimed,
    queryFn: devicesApi.getUnclaimed,
    staleTime: 10_000, // 10 seconds - unclaimed devices can appear quickly
  });
}

export function useClaimDevice() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: (id: string) => devicesApi.claim(id),
    onSuccess: () => {
      // Invalidate both devices and unclaimed queries
      queryClient.invalidateQueries({ queryKey: queryKeys.devices });
      queryClient.invalidateQueries({ queryKey: queryKeys.unclaimed });
    },
  });
}

export function useUpdateDevice() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({ id, data }: { id: string; data: { name?: string } }) =>
      devicesApi.update(id, data),
    onSuccess: (_, { id }) => {
      queryClient.invalidateQueries({ queryKey: queryKeys.devices });
      queryClient.invalidateQueries({ queryKey: queryKeys.device(id) });
    },
  });
}

export function useDeleteDevice() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: (id: string) => devicesApi.delete(id),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: queryKeys.devices });
    },
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// Telemetry Hooks
// ─────────────────────────────────────────────────────────────────────────────

export function useLatestTelemetry() {
  return useQuery<LatestTelemetry[]>({
    queryKey: queryKeys.telemetryLatest,
    queryFn: telemetryApi.getLatest,
    staleTime: 10_000, // 10 seconds
    refetchInterval: 30_000, // Refetch every 30 seconds
  });
}

export function useTelemetryHistory(
  deviceId: string | undefined,
  options?: { from?: string; to?: string; limit?: number }
) {
  return useQuery<Telemetry[]>({
    queryKey: [...queryKeys.telemetryHistory(deviceId!), options],
    queryFn: () => telemetryApi.getHistory(deviceId!, options),
    enabled: !!deviceId,
    staleTime: 60_000, // 1 minute
  });
}

export function useTelemetryAggregates(
  deviceId: string | undefined,
  options?: { from?: string; to?: string; bucket?: string }
) {
  return useQuery<TelemetryAggregate[]>({
    queryKey: [...queryKeys.telemetryAggregates(deviceId!), options],
    queryFn: () => telemetryApi.getAggregates(deviceId!, options),
    enabled: !!deviceId,
    staleTime: 60_000, // 1 minute
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// Hue Hooks
// ─────────────────────────────────────────────────────────────────────────────

export function useHueRooms(deviceId: string | undefined) {
  return useQuery<HueRoom[]>({
    queryKey: queryKeys.hueRooms(deviceId!),
    queryFn: () => hueApi.getRooms(deviceId!),
    enabled: !!deviceId,
    staleTime: 10_000,
  });
}

export function useHueToggle() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({
      deviceId,
      roomId,
      isOn,
    }: {
      deviceId: string;
      roomId: string;
      isOn: boolean;
    }) => hueApi.toggleRoom(deviceId, roomId, isOn),
    onSuccess: (_, { deviceId }) => {
      // Optimistic update will come via WebSocket, but invalidate to be safe
      queryClient.invalidateQueries({ queryKey: queryKeys.hueRooms(deviceId) });
    },
  });
}

export function useHueBrightness() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({
      deviceId,
      roomId,
      brightness,
      isOn,
    }: {
      deviceId: string;
      roomId: string;
      brightness: number;
      isOn?: boolean;
    }) => hueApi.setBrightness(deviceId, roomId, brightness, isOn),
    onSuccess: (_, { deviceId }) => {
      queryClient.invalidateQueries({ queryKey: queryKeys.hueRooms(deviceId) });
    },
  });
}

// ─────────────────────────────────────────────────────────────────────────────
// Tado Hooks
// ─────────────────────────────────────────────────────────────────────────────

export function useTadoRooms(deviceId: string | undefined) {
  return useQuery<TadoRoom[]>({
    queryKey: queryKeys.tadoRooms(deviceId!),
    queryFn: () => tadoApi.getRooms(deviceId!),
    enabled: !!deviceId,
    staleTime: 10_000,
  });
}

export function useTadoSetTemperature() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({
      deviceId,
      roomId,
      temperature,
    }: {
      deviceId: string;
      roomId: string;
      temperature: number;
    }) => tadoApi.setTemperature(deviceId, roomId, temperature),
    onSuccess: (_, { deviceId }) => {
      // Optimistic update will come via WebSocket, but invalidate to be safe
      queryClient.invalidateQueries({ queryKey: queryKeys.tadoRooms(deviceId) });
    },
  });
}
