export interface Device {
  id: string;
  deviceId: string; // MAC address format: A0B1C2D3E4F5
  name: string;
  ownerId: string;
  isOnline: boolean;
  lastSeenAt: Date | null;
  firmwareVersion?: string;
  createdAt: Date;
  updatedAt: Date;
}

export interface CreateDeviceDto {
  deviceId: string;
  name: string;
}

export interface UpdateDeviceDto {
  name?: string;
}

export interface DeviceStatus {
  deviceId: string;
  isOnline: boolean;
  lastSeenAt: Date | null;
  battery?: {
    percentage: number;
    voltage: number;
    isCharging: boolean;
  };
}
