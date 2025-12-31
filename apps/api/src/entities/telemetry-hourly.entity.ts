import {
  Entity,
  PrimaryGeneratedColumn,
  Column,
  Index,
  Unique,
} from 'typeorm';

/**
 * Pre-aggregated hourly telemetry data for long-term storage
 * Used for data older than 7 days (up to 90 days retention)
 */
@Entity('telemetry_hourly')
@Index(['deviceId', 'hour'])
@Unique(['deviceId', 'hour'])
export class TelemetryHourly {
  @PrimaryGeneratedColumn('uuid')
  id: string;

  @Column({ name: 'device_id', length: 20 })
  @Index()
  deviceId: string;

  @Column({ type: 'timestamptz' })
  hour: Date;

  @Column({ name: 'avg_co2', type: 'float', nullable: true })
  avgCo2: number | null;

  @Column({ name: 'min_co2', type: 'float', nullable: true })
  minCo2: number | null;

  @Column({ name: 'max_co2', type: 'float', nullable: true })
  maxCo2: number | null;

  @Column({ name: 'avg_temperature', type: 'float', nullable: true })
  avgTemperature: number | null;

  @Column({ name: 'min_temperature', type: 'float', nullable: true })
  minTemperature: number | null;

  @Column({ name: 'max_temperature', type: 'float', nullable: true })
  maxTemperature: number | null;

  @Column({ name: 'avg_humidity', type: 'float', nullable: true })
  avgHumidity: number | null;

  @Column({ name: 'min_humidity', type: 'float', nullable: true })
  minHumidity: number | null;

  @Column({ name: 'max_humidity', type: 'float', nullable: true })
  maxHumidity: number | null;

  @Column({ name: 'avg_battery', type: 'float', nullable: true })
  avgBattery: number | null;

  // BME688/BSEC2 aggregated data
  @Column({ name: 'avg_iaq', type: 'float', nullable: true })
  avgIaq: number | null;

  @Column({ name: 'min_iaq', type: 'float', nullable: true })
  minIaq: number | null;

  @Column({ name: 'max_iaq', type: 'float', nullable: true })
  maxIaq: number | null;

  @Column({ name: 'avg_pressure', type: 'float', nullable: true })
  avgPressure: number | null;

  @Column({ name: 'min_pressure', type: 'float', nullable: true })
  minPressure: number | null;

  @Column({ name: 'max_pressure', type: 'float', nullable: true })
  maxPressure: number | null;

  @Column({ name: 'avg_bme688_temperature', type: 'float', nullable: true })
  avgBme688Temperature: number | null;

  @Column({ name: 'avg_bme688_humidity', type: 'float', nullable: true })
  avgBme688Humidity: number | null;

  @Column({ name: 'sample_count', type: 'int', default: 0 })
  sampleCount: number;
}
