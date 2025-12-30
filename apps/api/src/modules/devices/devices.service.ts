import {
  Injectable,
  NotFoundException,
  ConflictException,
  Logger,
} from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository, IsNull } from 'typeorm';
import { Device } from '../../entities/device.entity';
import { CreateDeviceDto, UpdateDeviceDto } from './dto';

@Injectable()
export class DevicesService {
  private readonly logger = new Logger(DevicesService.name);

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

  /**
   * Find a device if user owns it OR if it's unclaimed
   */
  async findOneForUser(userId: string, id: string): Promise<Device> {
    const device = await this.deviceRepository.findOne({
      where: { id },
    });

    if (!device) {
      throw new NotFoundException('Device not found');
    }

    // User can access if they own it OR if it's unclaimed
    if (device.ownerId !== null && device.ownerId !== userId) {
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

  async updateFirmwareVersion(deviceId: string, firmwareVersion: string): Promise<void> {
    await this.deviceRepository.update(
      { deviceId },
      { firmwareVersion }
    );
  }

  /**
   * Find or auto-create an unclaimed device when it first connects via MQTT
   */
  async findOrCreateUnclaimed(deviceId: string): Promise<Device> {
    let device = await this.findByDeviceId(deviceId);

    if (!device) {
      // Auto-create unclaimed device with name based on last 4 chars of MAC
      device = this.deviceRepository.create({
        deviceId,
        name: `Device ${deviceId.slice(-4)}`,
        ownerId: null, // Unclaimed
        isOnline: false,
      });
      device = await this.deviceRepository.save(device);
      this.logger.log(`Auto-registered unclaimed device: ${deviceId}`);
    }

    return device;
  }

  /**
   * Get all unclaimed devices (visible to all users)
   */
  async findAllUnclaimed(): Promise<Device[]> {
    return this.deviceRepository.find({
      where: { ownerId: IsNull() },
      order: { lastSeenAt: 'DESC' },
    });
  }

  /**
   * Claim an unclaimed device for a user
   */
  async claimDevice(userId: string, id: string): Promise<Device> {
    const device = await this.deviceRepository.findOne({
      where: { id, ownerId: IsNull() },
    });

    if (!device) {
      throw new NotFoundException('Unclaimed device not found');
    }

    device.ownerId = userId;
    return this.deviceRepository.save(device);
  }

  /**
   * Get owner IDs for a device (for WebSocket broadcasting)
   */
  async getOwnersByDeviceId(deviceId: string): Promise<string[]> {
    const device = await this.findByDeviceId(deviceId);
    return device?.ownerId ? [device.ownerId] : [];
  }

  /**
   * Find all devices (both owned and unclaimed) for dashboard
   * Returns user's owned devices + unclaimed devices
   */
  async findAllForUser(userId: string): Promise<Device[]> {
    const [owned, unclaimed] = await Promise.all([
      this.findAllByOwner(userId),
      this.findAllUnclaimed(),
    ]);
    return [...owned, ...unclaimed];
  }
}
