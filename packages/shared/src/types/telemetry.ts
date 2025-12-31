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
  // STCC4 sensor data
  co2?: number;
  temperature?: number;
  humidity?: number;
  battery?: number;
  // BME688/BSEC2 sensor data
  iaq?: number;
  iaqAccuracy?: number;
  pressure?: number;
  gasResistance?: number;
  bme688Temperature?: number;
  bme688Humidity?: number;
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
  // STCC4 sensor data
  co2: number | null;
  temperature: number | null;
  humidity: number | null;
  battery: number | null;
  // BME688/BSEC2 sensor data
  iaq: number | null;
  iaqAccuracy: number | null;
  pressure: number | null;
  bme688Temperature: number | null;
  bme688Humidity: number | null;
}
