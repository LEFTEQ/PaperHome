import { Injectable, NotFoundException, Inject, forwardRef } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Command, CommandStatus } from '../../entities/command.entity';
import { DevicesService } from '../devices/devices.service';
import { MqttService } from '../mqtt/mqtt.service';
import { CreateCommandDto } from './dto/create-command.dto';

@Injectable()
export class CommandsService {
  constructor(
    @InjectRepository(Command)
    private commandRepository: Repository<Command>,
    private devicesService: DevicesService,
    @Inject(forwardRef(() => MqttService))
    private mqttService: MqttService
  ) {}

  async sendCommand(userId: string, createDto: CreateCommandDto) {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === createDto.deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    const command = this.commandRepository.create({
      deviceId: createDto.deviceId,
      type: createDto.type,
      payload: createDto.payload,
      status: CommandStatus.PENDING,
    });

    const saved = await this.commandRepository.save(command);

    // Publish to MQTT
    await this.mqttService.publishCommand(saved);

    return saved;
  }

  async getHistory(userId: string, deviceId: string, limit: number) {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    return this.commandRepository.find({
      where: { deviceId },
      order: { createdAt: 'DESC' },
      take: limit,
    });
  }

  async acknowledgeCommand(commandId: string, success: boolean, errorMessage?: string) {
    await this.commandRepository.update(commandId, {
      status: success ? CommandStatus.ACKNOWLEDGED : CommandStatus.FAILED,
      acknowledgedAt: new Date(),
      errorMessage: errorMessage || null,
    });
  }

  async markSent(commandId: string) {
    await this.commandRepository.update(commandId, {
      status: CommandStatus.SENT,
      sentAt: new Date(),
    });
  }
}
