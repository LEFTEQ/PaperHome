import { DataSource, DataSourceOptions } from 'typeorm';
import * as dotenv from 'dotenv';
import { join } from 'path';
import {
  User,
  RefreshToken,
  Device,
  Telemetry,
  TelemetryHourly,
  HueState,
  TadoState,
  TadoZoneMapping,
  Command,
} from '../entities/index';

// Load environment variables from monorepo root
dotenv.config({ path: join(__dirname, '../../../../.env') });

export const dataSourceOptions: DataSourceOptions = {
  type: 'postgres',
  host: process.env.DB_HOST || 'localhost',
  port: parseInt(process.env.DB_PORT || '5433', 10),
  username: process.env.DB_USER || 'paperhome',
  password: process.env.DB_PASSWORD || 'paperhome_dev_password',
  database: process.env.DB_NAME || 'paperhome',
  entities: [
    User,
    RefreshToken,
    Device,
    Telemetry,
    TelemetryHourly,
    HueState,
    TadoState,
    TadoZoneMapping,
    Command,
  ],
  migrations: [join(__dirname, 'migrations', '*{.ts,.js}')],
  synchronize: process.env.DB_SYNCHRONIZE === 'true',
  logging: process.env.NODE_ENV === 'development',
  ssl: process.env.DB_SSL === 'true' ? { rejectUnauthorized: false } : false,
};

const dataSource = new DataSource(dataSourceOptions);

export default dataSource;
