import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  Index,
} from 'typeorm';

@Entity('tado_states')
@Index(['deviceId', 'time'])
export class TadoState {
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

  @Column({ name: 'current_temp', type: 'float' })
  currentTemp: number; // Celsius

  @Column({ name: 'target_temp', type: 'float' })
  targetTemp: number; // Celsius

  @Column({ type: 'float' })
  humidity: number; // percentage

  @Column({ name: 'is_heating' })
  isHeating: boolean;

  @Column({ name: 'heating_power', type: 'int' })
  heatingPower: number; // 0-100
}
