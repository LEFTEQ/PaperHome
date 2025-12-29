import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  CreateDateColumn,
  Index,
} from 'typeorm';

export enum CommandType {
  HUE_SET_ROOM = 'hue_set_room',
  TADO_SET_TEMP = 'tado_set_temp',
  DEVICE_REBOOT = 'device_reboot',
  DEVICE_OTA_UPDATE = 'device_ota_update',
}

export enum CommandStatus {
  PENDING = 'pending',
  SENT = 'sent',
  ACKNOWLEDGED = 'acknowledged',
  FAILED = 'failed',
  TIMEOUT = 'timeout',
}

@Entity('commands')
export class Command {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ name: 'device_id', length: 20 })
  @Index()
  deviceId: string;

  @Column({ type: 'enum', enum: CommandType })
  type: CommandType;

  @Column({ type: 'jsonb' })
  payload: Record<string, unknown>;

  @Column({ type: 'enum', enum: CommandStatus, default: CommandStatus.PENDING })
  status: CommandStatus;

  @Column({ name: 'sent_at', type: 'timestamptz', nullable: true })
  sentAt: Date | null;

  @Column({ name: 'acknowledged_at', type: 'timestamptz', nullable: true })
  acknowledgedAt: Date | null;

  @Column({ name: 'error_message', type: 'text', nullable: true })
  errorMessage: string | null;

  @CreateDateColumn({ name: 'created_at' })
  createdAt: Date;
}
