// WebSocket hooks
export {
  useWebSocket,
  useWebSocketMessage,
  type UseWebSocketOptions,
  type UseWebSocketReturn,
} from './use-websocket';

// Device subscription hooks (WebSocket-based state)
export {
  useDeviceSubscription,
  useDevicesSubscription,
  useTelemetryHistory as useWsTelemetryHistory,
  type DeviceState,
  type TelemetryPoint,
  type UseDeviceSubscriptionOptions,
} from './use-device-subscription';

// Notification hooks
export {
  useNotifications,
  useNotificationContext,
  NotificationProvider,
  type Notification,
  type UseNotificationsReturn,
} from './use-notifications';

// API hooks (React Query)
export {
  useDevices,
  useDevice,
  useUnclaimedDevices,
  useClaimDevice,
  useUpdateDevice,
  useDeleteDevice,
  useLatestTelemetry,
  useTelemetryHistory,
  useTelemetryAggregates,
  useHueRooms,
  useHueToggle,
  useHueBrightness,
  useTadoRooms,
  useTadoSetTemperature,
  queryKeys,
} from './use-devices';

// Real-time updates (WebSocket + React Query integration)
export { useRealtimeUpdates, useDeviceSubscription as useDeviceRealtimeSubscription } from './use-realtime-updates';

// Utility hooks
export { useDebouncedCallback } from './use-debounce';
