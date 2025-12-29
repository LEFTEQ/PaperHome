export enum CommandType {
  HUE_SET_ROOM = 'hue_set_room',
  TADO_SET_TEMP = 'tado_set_temp',
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
