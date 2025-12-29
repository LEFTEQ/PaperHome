import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { TadoState } from '../../entities/tado-state.entity';
import { DevicesService } from '../devices/devices.service';

interface HistoryQuery {
  from?: Date;
  to?: Date;
  roomId?: string;
}

@Injectable()
export class TadoService {
  constructor(
    @InjectRepository(TadoState)
    private tadoStateRepository: Repository<TadoState>,
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
}
