import {
  Injectable,
  OnModuleInit,
  OnModuleDestroy,
  Logger,
  Inject,
  forwardRef,
} from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import * as mqtt from 'mqtt';
import { DevicesService } from '../devices/devices.service';
import { TelemetryService } from '../telemetry/telemetry.service';
import { HueService } from '../hue/hue.service';
import { TadoService } from '../tado/tado.service';
import { CommandsService } from '../commands/commands.service';
import { Command } from '../../entities/command.entity';
import {
  SUBSCRIPTION_PATTERNS,
  extractDeviceId,
  extractMessageType,
  buildTopic,
  SERVER_PUBLISH_TOPICS,
} from '@paperhome/shared';

@Injectable()
export class MqttService implements OnModuleInit, OnModuleDestroy {
  private client: mqtt.MqttClient;
  private readonly logger = new Logger(MqttService.name);

  constructor(
    private configService: ConfigService,
    private devicesService: DevicesService,
    private telemetryService: TelemetryService,
    private hueService: HueService,
    private tadoService: TadoService,
    @Inject(forwardRef(() => CommandsService))
    private commandsService: CommandsService
  ) {}

  async onModuleInit() {
    const brokerUrl =
      this.configService.get<string>('MQTT_BROKER_URL') || 'mqtt://localhost:1883';
    const username = this.configService.get<string>('MQTT_USERNAME');
    const password = this.configService.get<string>('MQTT_PASSWORD');

    this.client = mqtt.connect(brokerUrl, {
      clientId: 'paperhome-server',
      clean: true,
      reconnectPeriod: 5000,
      username,
      password,
    });

    this.client.on('connect', () => {
      this.logger.log('Connected to MQTT broker');
      this.subscribeToTopics();
    });

    this.client.on('message', async (topic, payload) => {
      try {
        await this.handleMessage(topic, payload);
      } catch (error) {
        this.logger.error(`Error handling message on topic ${topic}:`, error);
      }
    });

    this.client.on('error', (error) => {
      this.logger.error('MQTT connection error:', error);
    });
  }

  async onModuleDestroy() {
    if (this.client) {
      this.client.end();
      this.logger.log('Disconnected from MQTT broker');
    }
  }

  private subscribeToTopics() {
    const topics = [
      SUBSCRIPTION_PATTERNS.ALL_TELEMETRY,
      SUBSCRIPTION_PATTERNS.ALL_STATUS,
      SUBSCRIPTION_PATTERNS.ALL_HUE_STATE,
      SUBSCRIPTION_PATTERNS.ALL_TADO_STATE,
      SUBSCRIPTION_PATTERNS.ALL_COMMAND_ACK,
    ];

    for (const topic of topics) {
      this.client.subscribe(topic, (err) => {
        if (err) {
          this.logger.error(`Failed to subscribe to ${topic}`, err);
        } else {
          this.logger.log(`Subscribed to ${topic}`);
        }
      });
    }
  }

  private async handleMessage(topic: string, payload: Buffer) {
    const deviceId = extractDeviceId(topic);
    const messageType = extractMessageType(topic);

    if (!deviceId || !messageType) {
      this.logger.warn(`Invalid topic format: ${topic}`);
      return;
    }

    const data = JSON.parse(payload.toString());

    switch (messageType) {
      case 'telemetry':
        await this.handleTelemetry(deviceId, data);
        break;
      case 'status':
        await this.handleStatus(deviceId, data);
        break;
      case 'hue/state':
        await this.handleHueState(deviceId, data);
        break;
      case 'tado/state':
        await this.handleTadoState(deviceId, data);
        break;
      case 'command/ack':
        await this.handleCommandAck(deviceId, data);
        break;
      default:
        this.logger.debug(`Unhandled message type: ${messageType}`);
    }
  }

  private async handleTelemetry(deviceId: string, data: any) {
    await this.telemetryService.create({
      deviceId,
      co2: data.co2,
      temperature: data.temperature,
      humidity: data.humidity,
      battery: data.battery,
    });

    // Update device online status
    await this.devicesService.updateStatus(deviceId, true);

    this.logger.debug(`Telemetry received from ${deviceId}`);
  }

  private async handleStatus(deviceId: string, data: any) {
    const isOnline = data.status === 'online';
    await this.devicesService.updateStatus(deviceId, isOnline);
    this.logger.log(`Device ${deviceId} is ${isOnline ? 'online' : 'offline'}`);
  }

  private async handleHueState(deviceId: string, data: any) {
    // data can be a single room or array of rooms
    const rooms = Array.isArray(data) ? data : [data];

    for (const room of rooms) {
      await this.hueService.saveState({
        deviceId,
        roomId: room.roomId,
        roomName: room.roomName,
        isOn: room.isOn,
        brightness: room.brightness,
      });
    }

    this.logger.debug(`Hue state received from ${deviceId}: ${rooms.length} rooms`);
  }

  private async handleTadoState(deviceId: string, data: any) {
    // data can be a single room or array of rooms
    const rooms = Array.isArray(data) ? data : [data];

    for (const room of rooms) {
      await this.tadoService.saveState({
        deviceId,
        roomId: room.roomId,
        roomName: room.roomName,
        currentTemp: room.currentTemp,
        targetTemp: room.targetTemp,
        humidity: room.humidity,
        isHeating: room.isHeating,
        heatingPower: room.heatingPower,
      });
    }

    this.logger.debug(`Tado state received from ${deviceId}: ${rooms.length} rooms`);
  }

  private async handleCommandAck(deviceId: string, data: any) {
    await this.commandsService.acknowledgeCommand(
      data.commandId,
      data.success,
      data.errorMessage
    );

    this.logger.log(
      `Command ${data.commandId} ${data.success ? 'acknowledged' : 'failed'}`
    );
  }

  async publishCommand(command: Command): Promise<void> {
    const topic = buildTopic(command.deviceId, SERVER_PUBLISH_TOPICS.COMMAND);
    const payload = JSON.stringify({
      commandId: command.id,
      type: command.type,
      payload: command.payload,
    });

    this.client.publish(topic, payload, { qos: 1 }, (err) => {
      if (err) {
        this.logger.error(`Failed to publish command to ${topic}`, err);
      } else {
        this.commandsService.markSent(command.id);
        this.logger.log(`Command ${command.id} published to ${topic}`);
      }
    });
  }

  getConnectionStatus(): { connected: boolean; reconnecting: boolean } {
    return {
      connected: this.client?.connected ?? false,
      reconnecting: this.client?.reconnecting ?? false,
    };
  }
}
