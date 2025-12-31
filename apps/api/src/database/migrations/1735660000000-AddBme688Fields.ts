import { MigrationInterface, QueryRunner } from 'typeorm';

export class AddBme688Fields1735660000000 implements MigrationInterface {
  name = 'AddBme688Fields1735660000000';

  public async up(queryRunner: QueryRunner): Promise<void> {
    // Add BME688 columns to telemetry table
    await queryRunner.query(`
      ALTER TABLE telemetry
      ADD COLUMN IF NOT EXISTS iaq SMALLINT,
      ADD COLUMN IF NOT EXISTS iaq_accuracy SMALLINT,
      ADD COLUMN IF NOT EXISTS pressure FLOAT,
      ADD COLUMN IF NOT EXISTS gas_resistance INTEGER,
      ADD COLUMN IF NOT EXISTS bme688_temperature FLOAT,
      ADD COLUMN IF NOT EXISTS bme688_humidity FLOAT;
    `);

    // Add BME688 aggregated columns to telemetry_hourly table
    await queryRunner.query(`
      ALTER TABLE telemetry_hourly
      ADD COLUMN IF NOT EXISTS avg_iaq FLOAT,
      ADD COLUMN IF NOT EXISTS min_iaq FLOAT,
      ADD COLUMN IF NOT EXISTS max_iaq FLOAT,
      ADD COLUMN IF NOT EXISTS avg_pressure FLOAT,
      ADD COLUMN IF NOT EXISTS min_pressure FLOAT,
      ADD COLUMN IF NOT EXISTS max_pressure FLOAT,
      ADD COLUMN IF NOT EXISTS avg_bme688_temperature FLOAT,
      ADD COLUMN IF NOT EXISTS avg_bme688_humidity FLOAT;
    `);
  }

  public async down(queryRunner: QueryRunner): Promise<void> {
    // Remove BME688 columns from telemetry table
    await queryRunner.query(`
      ALTER TABLE telemetry
      DROP COLUMN IF EXISTS iaq,
      DROP COLUMN IF EXISTS iaq_accuracy,
      DROP COLUMN IF EXISTS pressure,
      DROP COLUMN IF EXISTS gas_resistance,
      DROP COLUMN IF EXISTS bme688_temperature,
      DROP COLUMN IF EXISTS bme688_humidity;
    `);

    // Remove BME688 columns from telemetry_hourly table
    await queryRunner.query(`
      ALTER TABLE telemetry_hourly
      DROP COLUMN IF EXISTS avg_iaq,
      DROP COLUMN IF EXISTS min_iaq,
      DROP COLUMN IF EXISTS max_iaq,
      DROP COLUMN IF EXISTS avg_pressure,
      DROP COLUMN IF EXISTS min_pressure,
      DROP COLUMN IF EXISTS max_pressure,
      DROP COLUMN IF EXISTS avg_bme688_temperature,
      DROP COLUMN IF EXISTS avg_bme688_humidity;
    `);
  }
}
