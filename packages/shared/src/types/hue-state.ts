export interface HueState {
  id: string;
  time: Date;
  deviceId: string;
  roomId: string;
  roomName: string;
  isOn: boolean;
  brightness: number; // 0-100
}

export interface HueStatePayload {
  roomId: string;
  roomName: string;
  isOn: boolean;
  brightness: number;
}

export interface HueRoom {
  roomId: string;
  roomName: string;
  isOn: boolean;
  brightness: number;
  lastUpdated: Date;
}

export interface HueRoomCommand {
  roomId: string;
  isOn?: boolean;
  brightness?: number;
}
