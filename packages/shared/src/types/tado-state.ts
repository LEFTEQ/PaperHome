export interface TadoState {
  id: string;
  time: Date;
  deviceId: string;
  roomId: string;
  roomName: string;
  currentTemp: number; // Celsius
  targetTemp: number; // Celsius
  humidity: number; // percentage
  isHeating: boolean;
  heatingPower: number; // 0-100 percentage
}

export interface TadoStatePayload {
  roomId: string;
  roomName: string;
  currentTemp: number;
  targetTemp: number;
  humidity: number;
  isHeating: boolean;
  heatingPower: number;
}

export interface TadoRoom {
  roomId: string;
  roomName: string;
  currentTemp: number;
  targetTemp: number;
  humidity: number;
  isHeating: boolean;
  heatingPower: number;
  lastUpdated: Date;
}

export interface TadoRoomCommand {
  roomId: string;
  targetTemp?: number;
}
