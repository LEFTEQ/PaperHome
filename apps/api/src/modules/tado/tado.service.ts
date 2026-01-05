import { Injectable, NotFoundException, BadRequestException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { TadoState } from '../../entities/tado-state.entity';
import { TadoZoneMapping } from '../../entities/tado-zone-mapping.entity';
import { DevicesService } from '../devices/devices.service';

interface HistoryQuery {
  from?: Date;
  to?: Date;
  roomId?: string;
}

export interface CreateZoneMappingDto {
  tadoZoneId: number;
  tadoZoneName: string;
  targetTemperature?: number;
  autoAdjustEnabled?: boolean;
}

export interface UpdateZoneMappingDto {
  targetTemperature?: number;
  autoAdjustEnabled?: boolean;
  hysteresis?: number;
}

@Injectable()
export class TadoService {
  constructor(
    @InjectRepository(TadoState)
    private tadoStateRepository: Repository<TadoState>,
    @InjectRepository(TadoZoneMapping)
    private zoneMappingRepository: Repository<TadoZoneMapping>,
    private devicesService: DevicesService
  ) {}

  async saveState(data: Partial<TadoState>): Promise<TadoState> {
    const state = this.tadoStateRepository.create({
      ...data,
      time: data.time || new Date(),
    });
    return this.tadoStateRepository.save(state);
  }

  async getLatestRoomStates(userId: string, deviceId: string) {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // Get latest state for each room
    const latestStates = await this.tadoStateRepository
      .createQueryBuilder('ts')
      .distinctOn(['ts.room_id'])
      .where('ts.device_id = :deviceId', { deviceId })
      .orderBy('ts.room_id', 'ASC')
      .addOrderBy('ts.time', 'DESC')
      .getMany();

    return latestStates.map((state) => ({
      roomId: state.roomId,
      roomName: state.roomName,
      currentTemp: state.currentTemp,
      targetTemp: state.targetTemp,
      humidity: state.humidity,
      isHeating: state.isHeating,
      heatingPower: state.heatingPower,
      lastUpdated: state.time,
    }));
  }

  async getHistory(userId: string, deviceId: string, query: HistoryQuery) {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    const qb = this.tadoStateRepository
      .createQueryBuilder('ts')
      .where('ts.device_id = :deviceId', { deviceId });

    if (query.from) {
      qb.andWhere('ts.time >= :from', { from: query.from });
    }
    if (query.to) {
      qb.andWhere('ts.time <= :to', { to: query.to });
    }
    if (query.roomId) {
      qb.andWhere('ts.room_id = :roomId', { roomId: query.roomId });
    }

    return qb.orderBy('ts.time', 'DESC').take(1000).getMany();
  }

  // ==================== Zone Mapping CRUD ====================

  async getZoneMappings(userId: string, deviceId: string): Promise<TadoZoneMapping[]> {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    return this.zoneMappingRepository.find({
      where: { deviceId },
      order: { tadoZoneName: 'ASC' },
    });
  }

  async createZoneMapping(
    userId: string,
    deviceId: string,
    dto: CreateZoneMappingDto
  ): Promise<TadoZoneMapping> {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // Check if mapping already exists
    const existing = await this.zoneMappingRepository.findOne({
      where: { deviceId, tadoZoneId: dto.tadoZoneId },
    });
    if (existing) {
      throw new BadRequestException('Zone mapping already exists for this device');
    }

    const mapping = this.zoneMappingRepository.create({
      deviceId,
      tadoZoneId: dto.tadoZoneId,
      tadoZoneName: dto.tadoZoneName,
      targetTemperature: dto.targetTemperature ?? 21.0,
      autoAdjustEnabled: dto.autoAdjustEnabled ?? false,
      hysteresis: 0.5, // Fixed default
    });

    return this.zoneMappingRepository.save(mapping);
  }

  async updateZoneMapping(
    userId: string,
    mappingId: string,
    dto: UpdateZoneMappingDto
  ): Promise<TadoZoneMapping> {
    const mapping = await this.zoneMappingRepository.findOne({
      where: { id: mappingId },
    });
    if (!mapping) {
      throw new NotFoundException('Zone mapping not found');
    }

    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === mapping.deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // Update fields
    if (dto.targetTemperature !== undefined) {
      if (dto.targetTemperature < 5 || dto.targetTemperature > 30) {
        throw new BadRequestException('Target temperature must be between 5 and 30°C');
      }
      mapping.targetTemperature = dto.targetTemperature;
    }
    if (dto.autoAdjustEnabled !== undefined) {
      mapping.autoAdjustEnabled = dto.autoAdjustEnabled;
    }
    if (dto.hysteresis !== undefined) {
      if (dto.hysteresis < 0.1 || dto.hysteresis > 2.0) {
        throw new BadRequestException('Hysteresis must be between 0.1 and 2.0°C');
      }
      mapping.hysteresis = dto.hysteresis;
    }

    return this.zoneMappingRepository.save(mapping);
  }

  async deleteZoneMapping(userId: string, mappingId: string): Promise<void> {
    const mapping = await this.zoneMappingRepository.findOne({
      where: { id: mappingId },
    });
    if (!mapping) {
      throw new NotFoundException('Zone mapping not found');
    }

    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === mapping.deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    await this.zoneMappingRepository.remove(mapping);
  }

  async getZoneMappingByDeviceAndZone(
    deviceId: string,
    zoneId: number
  ): Promise<TadoZoneMapping | null> {
    return this.zoneMappingRepository.findOne({
      where: { deviceId, tadoZoneId: zoneId },
    });
  }

  async getAllMappingsForDevice(deviceId: string): Promise<TadoZoneMapping[]> {
    return this.zoneMappingRepository.find({
      where: { deviceId },
    });
  }
}
