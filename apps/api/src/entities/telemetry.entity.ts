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

  // STCC4 sensor data
  @Column({ type: 'float', nullable: true })
  co2: number | null; // ppm

  @Column({ type: 'float', nullable: true })
  temperature: number | null; // Celsius

  @Column({ type: 'float', nullable: true })
  humidity: number | null; // percentage

  @Column({ type: 'float', nullable: true })
  battery: number | null; // percentage

  // BME688/BSEC2 sensor data
  @Column({ type: 'smallint', nullable: true })
  iaq: number | null; // Indoor Air Quality index (0-500)

  @Column({ name: 'iaq_accuracy', type: 'smallint', nullable: true })
  iaqAccuracy: number | null; // IAQ accuracy (0-3)

  @Column({ type: 'float', nullable: true })
  pressure: number | null; // hPa

  @Column({ name: 'gas_resistance', type: 'integer', nullable: true })
  gasResistance: number | null; // Ohms

  @Column({ name: 'bme688_temperature', type: 'float', nullable: true })
  bme688Temperature: number | null; // Celsius

  @Column({ name: 'bme688_humidity', type: 'float', nullable: true })
  bme688Humidity: number | null; // percentage
}
