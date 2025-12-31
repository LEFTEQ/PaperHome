/**
 * PaperHome API Client
 */

import axios, { AxiosError, InternalAxiosRequestConfig } from 'axios';
import { getAccessToken, getRefreshToken, setTokens, clearTokens, isTokenExpired } from './client';

// ─────────────────────────────────────────────────────────────────────────────
// Types
// ─────────────────────────────────────────────────────────────────────────────

export interface Device {
  id: string;
  deviceId: string;
  name: string;
  ownerId: string | null;
  isOnline: boolean;
  lastSeenAt: string | null;
  firmwareVersion: string | null;
  createdAt: string;
  updatedAt: string;
}

export interface Telemetry {
  id: string;
  time: string;
  deviceId: string;
  // STCC4 sensor data
  co2: number | null;
  temperature: number | null;
  humidity: number | null;
  battery: number | null;
  // BME688/BSEC2 sensor data
  iaq: number | null;
  iaqAccuracy: number | null;
  pressure: number | null;
  gasResistance: number | null;
  bme688Temperature: number | null;
  bme688Humidity: number | null;
}

export interface LatestTelemetry {
  deviceId: string;
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
  time: string;
}

export interface HueRoom {
  id: string;
  roomId: string;
  roomName: string;
  isOn: boolean;
  brightness: number;
  time: string;
}

export interface TadoRoom {
  id: string;
  roomId: string;
  roomName: string;
  currentTemp: number;
  targetTemp: number;
  humidity: number;
  isHeating: boolean;
  heatingPower: number;
  time: string;
}

export interface TelemetryAggregate {
  time: string;
  // STCC4 aggregates
  avgCo2: number | null;
  avgTemperature: number | null;
  avgHumidity: number | null;
  avgBattery?: number | null;
  minCo2?: number | null;
  maxCo2?: number | null;
  minTemperature?: number | null;
  maxTemperature?: number | null;
  // BME688 aggregates
  avgIaq?: number | null;
  minIaq?: number | null;
  maxIaq?: number | null;
  avgPressure?: number | null;
  minPressure?: number | null;
  maxPressure?: number | null;
  avgBme688Temperature?: number | null;
  avgBme688Humidity?: number | null;
}

export interface AuthResponse {
  accessToken: string;
  refreshToken: string;
  expiresIn: number;
  user: {
    id: string;
    email: string;
    name: string;
  };
}

// ─────────────────────────────────────────────────────────────────────────────
// Axios Instance
// ─────────────────────────────────────────────────────────────────────────────

const API_URL = import.meta.env.VITE_API_URL || '';

export const api = axios.create({
  baseURL: `${API_URL}/api/v1`,
  headers: {
    'Content-Type': 'application/json',
  },
});

// Request interceptor - add auth token
api.interceptors.request.use(async (config: InternalAxiosRequestConfig) => {
  const token = getAccessToken();

  if (token) {
    // Check if token is expired and we have a refresh token
    if (isTokenExpired()) {
      const refreshToken = getRefreshToken();
      if (refreshToken) {
        try {
          const response = await axios.post<AuthResponse>(`${API_URL}/api/v1/auth/refresh`, {
            refreshToken,
          });
          setTokens(
            response.data.accessToken,
            response.data.refreshToken,
            response.data.expiresIn
          );
          config.headers.Authorization = `Bearer ${response.data.accessToken}`;
        } catch {
          clearTokens();
          window.location.href = '/login';
          return config;
        }
      }
    } else {
      config.headers.Authorization = `Bearer ${token}`;
    }
  }

  return config;
});

// Response interceptor - handle 401
api.interceptors.response.use(
  (response) => response,
  (error: AxiosError) => {
    if (error.response?.status === 401) {
      clearTokens();
      window.location.href = '/login';
    }
    return Promise.reject(error);
  }
);

// ─────────────────────────────────────────────────────────────────────────────
// Auth API
// ─────────────────────────────────────────────────────────────────────────────

export const authApi = {
  login: async (email: string, password: string): Promise<AuthResponse> => {
    const response = await api.post<AuthResponse>('/auth/login', { email, password });
    return response.data;
  },

  register: async (email: string, password: string, name: string): Promise<AuthResponse> => {
    const response = await api.post<AuthResponse>('/auth/register', { email, password, name });
    return response.data;
  },

  refresh: async (refreshToken: string): Promise<AuthResponse> => {
    const response = await api.post<AuthResponse>('/auth/refresh', { refreshToken });
    return response.data;
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Devices API
// ─────────────────────────────────────────────────────────────────────────────

export const devicesApi = {
  getAll: async (): Promise<Device[]> => {
    const response = await api.get<Device[]>('/devices');
    return response.data;
  },

  getUnclaimed: async (): Promise<Device[]> => {
    const response = await api.get<Device[]>('/devices/unclaimed');
    return response.data;
  },

  getById: async (id: string): Promise<Device> => {
    const response = await api.get<Device>(`/devices/${id}`);
    return response.data;
  },

  claim: async (id: string): Promise<Device> => {
    const response = await api.post<Device>(`/devices/${id}/claim`);
    return response.data;
  },

  update: async (id: string, data: { name?: string }): Promise<Device> => {
    const response = await api.patch<Device>(`/devices/${id}`, data);
    return response.data;
  },

  delete: async (id: string): Promise<void> => {
    await api.delete(`/devices/${id}`);
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Telemetry API
// ─────────────────────────────────────────────────────────────────────────────

export const telemetryApi = {
  getLatest: async (): Promise<LatestTelemetry[]> => {
    const response = await api.get<LatestTelemetry[]>('/telemetry/latest');
    return response.data;
  },

  getHistory: async (
    deviceId: string,
    options?: { from?: string; to?: string; limit?: number }
  ): Promise<Telemetry[]> => {
    const params = new URLSearchParams();
    if (options?.from) params.set('from', options.from);
    if (options?.to) params.set('to', options.to);
    if (options?.limit) params.set('limit', options.limit.toString());

    const response = await api.get<Telemetry[]>(`/telemetry/${deviceId}?${params}`);
    return response.data;
  },

  getAggregates: async (
    deviceId: string,
    options?: { from?: string; to?: string; bucket?: string }
  ): Promise<TelemetryAggregate[]> => {
    const params = new URLSearchParams();
    if (options?.from) params.set('from', options.from);
    if (options?.to) params.set('to', options.to);
    if (options?.bucket) params.set('bucket', options.bucket);

    const response = await api.get<TelemetryAggregate[]>(`/telemetry/${deviceId}/aggregates?${params}`);
    return response.data;
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Hue API
// ─────────────────────────────────────────────────────────────────────────────

export const hueApi = {
  getRooms: async (deviceId: string): Promise<HueRoom[]> => {
    const response = await api.get<HueRoom[]>(`/hue/${deviceId}/rooms`);
    return response.data;
  },

  getHistory: async (
    deviceId: string,
    options?: { from?: string; to?: string; roomId?: string }
  ): Promise<HueRoom[]> => {
    const params = new URLSearchParams();
    if (options?.from) params.set('from', options.from);
    if (options?.to) params.set('to', options.to);
    if (options?.roomId) params.set('roomId', options.roomId);

    const response = await api.get<HueRoom[]>(`/hue/${deviceId}/rooms/history?${params}`);
    return response.data;
  },

  toggleRoom: async (deviceId: string, roomId: string, isOn: boolean): Promise<void> => {
    await api.post(`/hue/${deviceId}/rooms/${roomId}/toggle`, { isOn });
  },

  setBrightness: async (
    deviceId: string,
    roomId: string,
    brightness: number,
    isOn?: boolean
  ): Promise<void> => {
    await api.post(`/hue/${deviceId}/rooms/${roomId}/brightness`, {
      brightness,
      ...(isOn !== undefined && { isOn }),
    });
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Tado API
// ─────────────────────────────────────────────────────────────────────────────

export const tadoApi = {
  getRooms: async (deviceId: string): Promise<TadoRoom[]> => {
    const response = await api.get<TadoRoom[]>(`/tado/${deviceId}/rooms`);
    return response.data;
  },

  getHistory: async (
    deviceId: string,
    options?: { from?: string; to?: string; roomId?: string }
  ): Promise<TadoRoom[]> => {
    const params = new URLSearchParams();
    if (options?.from) params.set('from', options.from);
    if (options?.to) params.set('to', options.to);
    if (options?.roomId) params.set('roomId', options.roomId);

    const response = await api.get<TadoRoom[]>(`/tado/${deviceId}/rooms/history?${params}`);
    return response.data;
  },

  setTemperature: async (
    deviceId: string,
    roomId: string,
    temperature: number
  ): Promise<void> => {
    await api.post(`/tado/${deviceId}/rooms/${roomId}/temperature`, { temperature });
  },
};
