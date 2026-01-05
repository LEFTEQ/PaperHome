export enum CommandType {
  HUE_SET_ROOM = 'hue_set_room',
  TADO_SET_TEMP = 'tado_set_temp',
  TADO_SET_AUTO_ADJUST = 'tado_set_auto_adjust',
  TADO_SYNC_MAPPING = 'tado_sync_mapping',
  DEVICE_REBOOT = 'device_reboot',
  DEVICE_OTA_UPDATE = 'device_ota_update',
}

export enum CommandStatus {
  PENDING = 'pending',
  SENT = 'sent',
  ACKNOWLEDGED = 'acknowledged',
  FAILED = 'failed',
  TIMEOUT = 'timeout',
}

export interface Command {
  id: string;
  deviceId: string;
  type: CommandType;
  payload: Record<string, unknown>;
  status: CommandStatus;
  sentAt: Date | null;
  acknowledgedAt: Date | null;
  errorMessage: string | null;
  createdAt: Date;
}

export interface CreateCommandDto {
  deviceId: string;
  type: CommandType;
  payload: Record<string, unknown>;
}

export interface CommandAck {
  commandId: string;
  success: boolean;
  errorMessage?: string;
}

// Tado Auto-Adjust Command Payloads
export interface TadoSetAutoAdjustPayload {
  zoneId: number;
  enabled: boolean;
  targetTemperature: number;
  hysteresis?: number;
}

export interface TadoSyncMappingPayload {
  zoneId: number;
  zoneName: string;
  targetTemperature: number;
  autoAdjustEnabled: boolean;
  hysteresis: number;
}

// Auto-adjust status (published by firmware via MQTT)
export interface TadoAutoAdjustStatus {
  zoneId: number;
  autoAdjustEnabled: boolean;
  targetTemperature: number;
  esp32Temperature: number;
  tadoTargetTemperature: number;
  lastAdjustTime: number;
  adjustmentDelta?: number;
}
