import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  CreateDateColumn,
  UpdateDateColumn,
  Index,
  ManyToOne,
  JoinColumn,
  Unique,
} from 'typeorm';
import { Device } from './device.entity';

@Entity('tado_zone_mappings')
@Unique(['deviceId', 'tadoZoneId'])
@Index(['deviceId'])
export class TadoZoneMapping {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ name: 'device_id', length: 20 })
  deviceId: string; // ESP32 MAC address

  @Column({ name: 'tado_zone_id' })
  tadoZoneId: number; // Tado zone ID from HOPS API

  @Column({ name: 'tado_zone_name', length: 100 })
  tadoZoneName: string; // Display name for UI

  @Column({ name: 'target_temperature', type: 'float', default: 21.0 })
  targetTemperature: number; // User's desired room temperature (5-30°C)

  @Column({ name: 'auto_adjust_enabled', default: false })
  autoAdjustEnabled: boolean; // Whether auto-adjust is active

  @Column({ name: 'hysteresis', type: 'float', default: 0.5 })
  hysteresis: number; // Temperature threshold for adjustment (±0.5°C default)

  @CreateDateColumn({ name: 'created_at' })
  createdAt: Date;

  @UpdateDateColumn({ name: 'updated_at' })
  updatedAt: Date;

  @ManyToOne(() => Device, { onDelete: 'CASCADE' })
  @JoinColumn({ name: 'device_id', referencedColumnName: 'deviceId' })
  device: Device;
}
