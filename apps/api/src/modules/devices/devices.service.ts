import {
  Injectable,
  NotFoundException,
  ConflictException,
} from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import { Device } from '../../entities/device.entity';
import { CreateDeviceDto, UpdateDeviceDto } from './dto';

@Injectable()
export class DevicesService {
  constructor(
    @InjectRepository(Device)
    private deviceRepository: Repository<Device>
  ) {}

  async findAllByOwner(ownerId: string): Promise<Device[]> {
    return this.deviceRepository.find({
      where: { ownerId },
      order: { createdAt: 'DESC' },
    });
  }

  async create(ownerId: string, createDto: CreateDeviceDto): Promise<Device> {
    const existing = await this.deviceRepository.findOne({
      where: { deviceId: createDto.deviceId },
    });

    if (existing) {
      throw new ConflictException('Device already registered');
    }

    const device = this.deviceRepository.create({
      ...createDto,
      ownerId,
    });

    return this.deviceRepository.save(device);
  }

  async findOneByOwner(ownerId: string, id: string): Promise<Device> {
    const device = await this.deviceRepository.findOne({
      where: { id, ownerId },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    return device;
  }

  async findByDeviceId(deviceId: string): Promise<Device | null> {
    return this.deviceRepository.findOne({ where: { deviceId } });
  }

  async update(
    ownerId: string,
    id: string,
    updateDto: UpdateDeviceDto
  ): Promise<Device> {
    const device = await this.findOneByOwner(ownerId, id);
    Object.assign(device, updateDto);
    return this.deviceRepository.save(device);
  }

  async remove(ownerId: string, id: string): Promise<void> {
    const device = await this.findOneByOwner(ownerId, id);
    await this.deviceRepository.remove(device);
  }

  async updateStatus(deviceId: string, isOnline: boolean): Promise<void> {
    await this.deviceRepository.update(
      { deviceId },
      {
        isOnline,
        lastSeenAt: isOnline ? new Date() : undefined,
      }
    );
  }
}
