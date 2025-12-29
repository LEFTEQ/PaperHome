import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  CreateDateColumn,
  UpdateDateColumn,
  ManyToOne,
  JoinColumn,
  Index,
} from 'typeorm';
import { User } from './user.entity';

@Entity('devices')
export class Device {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ name: 'device_id', unique: true, length: 20 })
  @Index()
  deviceId: string; // MAC address format: A0B1C2D3E4F5

  @Column({ length: 100 })
  name: string;

  @Column({ name: 'owner_id' })
  ownerId: string;

  @Column({ name: 'is_online', default: false })
  isOnline: boolean;

  @Column({ name: 'last_seen_at', nullable: true })
  lastSeenAt: Date | null;

  @Column({ name: 'firmware_version', nullable: true, length: 20 })
  firmwareVersion: string | null;

  @CreateDateColumn({ name: 'created_at' })
  createdAt: Date;

  @UpdateDateColumn({ name: 'updated_at' })
  updatedAt: Date;

  @ManyToOne(() => User, (user) => user.devices)
  @JoinColumn({ name: 'owner_id' })
  owner: User;
}
