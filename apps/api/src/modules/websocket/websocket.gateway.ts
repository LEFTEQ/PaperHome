import {
  WebSocketGateway,
  WebSocketServer,
  OnGatewayConnection,
  OnGatewayDisconnect,
} from '@nestjs/websockets';
import { Server, Socket } from 'socket.io';
import { JwtService } from '@nestjs/jwt';
import { Logger } from '@nestjs/common';

interface TelemetryPayload {
  co2?: number;
  temperature?: number;
  humidity?: number;
  battery?: number;
}

interface StatusPayload {
  online: boolean;
  lastSeen: string;
}

interface HueRoomPayload {
  roomId: string;
  roomName: string;
  isOn: boolean;
  brightness: number;
}

interface TadoRoomPayload {
  roomId: string;
  roomName: string;
  currentTemp: number;
  targetTemp: number;
  humidity?: number;
  isHeating: boolean;
}

interface NotificationPayload {
  id: string;
  type: 'info' | 'warning' | 'error' | 'success';
  title: string;
  message?: string;
  deviceId?: string;
}

@WebSocketGateway({
  cors: {
    origin: '*',
    credentials: true,
  },
  path: '/ws',
  transports: ['websocket', 'polling'],
})
export class RealtimeGateway implements OnGatewayConnection, OnGatewayDisconnect {
  @WebSocketServer()
  server: Server;

  private readonly logger = new Logger(RealtimeGateway.name);
  private userSockets: Map<string, Set<string>> = new Map(); // userId -> socketIds

  constructor(private jwtService: JwtService) {}

  async handleConnection(client: Socket) {
    try {
      const token = client.handshake.query.token as string;

      if (!token) {
        this.logger.warn(`Connection without token: ${client.id}`);
        client.disconnect();
        return;
      }

      const payload = this.jwtService.verify(token);
      client.data.userId = payload.sub;

      // Track user's sockets
      if (!this.userSockets.has(payload.sub)) {
        this.userSockets.set(payload.sub, new Set());
      }
      this.userSockets.get(payload.sub)!.add(client.id);

      // Join user-specific room for targeted broadcasts
      client.join(`user:${payload.sub}`);

      this.logger.log(`Client connected: ${client.id} (user: ${payload.sub})`);
    } catch (error) {
      this.logger.warn(`Unauthorized connection attempt: ${client.id}`);
      client.disconnect();
    }
  }

  handleDisconnect(client: Socket) {
    const userId = client.data.userId;
    if (userId) {
      this.userSockets.get(userId)?.delete(client.id);
      if (this.userSockets.get(userId)?.size === 0) {
        this.userSockets.delete(userId);
      }
    }
    this.logger.log(`Client disconnected: ${client.id}`);
  }

  /**
   * Broadcast telemetry update to device owners
   */
  broadcastTelemetry(deviceId: string, ownerIds: string[], data: TelemetryPayload) {
    const message = {
      type: 'telemetry',
      deviceId,
      payload: data,
      timestamp: new Date().toISOString(),
    };

    for (const userId of ownerIds) {
      this.server.to(`user:${userId}`).emit('telemetry', message);
    }

    // Also broadcast to unclaimed device watchers (all connected users)
    if (ownerIds.length === 0) {
      this.server.emit('telemetry', message);
    }
  }

  /**
   * Broadcast device status change to owners
   */
  broadcastDeviceStatus(deviceId: string, ownerIds: string[], isOnline: boolean) {
    const message = {
      type: 'status',
      deviceId,
      payload: {
        online: isOnline,
        lastSeen: new Date().toISOString(),
      } as StatusPayload,
      timestamp: new Date().toISOString(),
    };

    for (const userId of ownerIds) {
      this.server.to(`user:${userId}`).emit('status', message);
    }

    // Broadcast unclaimed device status to everyone
    if (ownerIds.length === 0) {
      this.server.emit('status', message);
    }
  }

  /**
   * Broadcast Hue state update to device owners
   */
  broadcastHueState(deviceId: string, ownerIds: string[], rooms: HueRoomPayload[]) {
    const message = {
      type: 'hue_state',
      deviceId,
      payload: rooms,
      timestamp: new Date().toISOString(),
    };

    for (const userId of ownerIds) {
      this.server.to(`user:${userId}`).emit('hue_state', message);
    }

    if (ownerIds.length === 0) {
      this.server.emit('hue_state', message);
    }
  }

  /**
   * Broadcast Tado state update to device owners
   */
  broadcastTadoState(deviceId: string, ownerIds: string[], rooms: TadoRoomPayload[]) {
    const message = {
      type: 'tado_state',
      deviceId,
      payload: rooms,
      timestamp: new Date().toISOString(),
    };

    for (const userId of ownerIds) {
      this.server.to(`user:${userId}`).emit('tado_state', message);
    }

    if (ownerIds.length === 0) {
      this.server.emit('tado_state', message);
    }
  }

  /**
   * Send notification to a specific user
   */
  sendNotification(userId: string, notification: NotificationPayload) {
    const message = {
      type: 'notification',
      payload: notification,
      timestamp: new Date().toISOString(),
    };

    this.server.to(`user:${userId}`).emit('notification', message);
  }

  /**
   * Broadcast command acknowledgment
   */
  broadcastCommandAck(userId: string, commandId: string, success: boolean, errorMessage?: string) {
    const message = {
      type: 'command_ack',
      payload: {
        commandId,
        success,
        errorMessage,
      },
      timestamp: new Date().toISOString(),
    };

    this.server.to(`user:${userId}`).emit('command_ack', message);
  }

  /**
   * Get count of connected clients
   */
  getConnectedClientsCount(): number {
    return this.server?.sockets?.sockets?.size ?? 0;
  }

  /**
   * Get count of connected users
   */
  getConnectedUsersCount(): number {
    return this.userSockets.size;
  }
}
