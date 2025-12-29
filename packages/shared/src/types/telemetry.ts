export interface Telemetry {
  id: string;
  time: Date;
  deviceId: string;
  co2: number | null; // ppm
  temperature: number | null; // Celsius
  humidity: number | null; // percentage
  battery: number | null; // percentage
}

export interface TelemetryPayload {
  co2?: number;
  temperature?: number;
  humidity?: number;
  battery?: number;
}

export interface TelemetryQuery {
  deviceId: string;
  from?: Date;
  to?: Date;
  limit?: number;
}

export interface TelemetryAggregate {
  time: Date;
  avgCo2: number | null;
  avgTemperature: number | null;
  avgHumidity: number | null;
  minCo2: number | null;
  maxCo2: number | null;
  minTemperature: number | null;
  maxTemperature: number | null;
}

export interface LatestTelemetry {
  deviceId: string;
  deviceName: string;
  time: Date;
  co2: number | null;
  temperature: number | null;
  humidity: number | null;
  battery: number | null;
}
