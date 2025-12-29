import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  Index,
} from 'typeorm';

@Entity('hue_states')
@Index(['deviceId', 'time'])
export class HueState {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'timestamptz' })
  time: Date;

  @Column({ name: 'device_id', length: 20 })
  @Index()
  deviceId: string;

  @Column({ name: 'room_id', length: 50 })
  roomId: string;

  @Column({ name: 'room_name', length: 100 })
  roomName: string;

  @Column({ name: 'is_on' })
  isOn: boolean;

  @Column({ type: 'int' })
  brightness: number; // 0-100
}
