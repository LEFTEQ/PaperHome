import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { HueState } from '../../entities/hue-state.entity';
import { DevicesService } from '../devices/devices.service';

interface HistoryQuery {
  from?: Date;
  to?: Date;
  roomId?: string;
}

@Injectable()
export class HueService {
  constructor(
    @InjectRepository(HueState)
    private hueStateRepository: Repository<HueState>,
    private devicesService: DevicesService
  ) {}

  async saveState(data: Partial<HueState>): Promise<HueState> {
    const state = this.hueStateRepository.create({
      ...data,
      time: data.time || new Date(),
    });
    return this.hueStateRepository.save(state);
  }

  async getLatestRoomStates(userId: string, deviceId: string) {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // Get latest state for each room using subquery
    const latestStates = await this.hueStateRepository
      .createQueryBuilder('hs')
      .distinctOn(['hs.room_id'])
      .where('hs.device_id = :deviceId', { deviceId })
      .orderBy('hs.room_id', 'ASC')
      .addOrderBy('hs.time', 'DESC')
      .getMany();

    return latestStates.map((state) => ({
      roomId: state.roomId,
      roomName: state.roomName,
      isOn: state.isOn,
      brightness: state.brightness,
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

    const qb = this.hueStateRepository
      .createQueryBuilder('hs')
      .where('hs.device_id = :deviceId', { deviceId });

    if (query.from) {
      qb.andWhere('hs.time >= :from', { from: query.from });
    }
    if (query.to) {
      qb.andWhere('hs.time <= :to', { to: query.to });
    }
    if (query.roomId) {
      qb.andWhere('hs.room_id = :roomId', { roomId: query.roomId });
    }

    return qb.orderBy('hs.time', 'DESC').take(1000).getMany();
  }
}
