import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  Index,
} from 'typeorm';

@Entity('telemetry')
@Index(['deviceId', 'time'])
export class Telemetry {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ type: 'timestamptz' })
  time: Date;

  @Column({ name: 'device_id', length: 20 })
  @Index()
  deviceId: string;

  @Column({ type: 'float', nullable: true })
  co2: number | null; // ppm

  @Column({ type: 'float', nullable: true })
  temperature: number | null; // Celsius

  @Column({ type: 'float', nullable: true })
  humidity: number | null; // percentage

  @Column({ type: 'float', nullable: true })
  battery: number | null; // percentage
}
