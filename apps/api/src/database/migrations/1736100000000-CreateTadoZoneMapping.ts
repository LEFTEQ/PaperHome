import { MigrationInterface, QueryRunner } from 'typeorm';

export class CreateTadoZoneMapping1736100000000 implements MigrationInterface {
  name = 'CreateTadoZoneMapping1736100000000';

  public async up(queryRunner: QueryRunner): Promise<void> {
    // Create tado_zone_mappings table
    await queryRunner.query(`
      CREATE TABLE tado_zone_mappings (
        id UUID PRIMARY KEY DEFAULT uuid_generate_v4(),
        device_id VARCHAR(20) NOT NULL,
        tado_zone_id INTEGER NOT NULL,
        tado_zone_name VARCHAR(100) NOT NULL,
        target_temperature FLOAT DEFAULT 21.0,
        auto_adjust_enabled BOOLEAN DEFAULT FALSE,
        hysteresis FLOAT DEFAULT 0.5,
        created_at TIMESTAMPTZ DEFAULT NOW(),
        updated_at TIMESTAMPTZ DEFAULT NOW(),
        CONSTRAINT uq_device_zone UNIQUE(device_id, tado_zone_id),
        CONSTRAINT fk_device FOREIGN KEY (device_id)
          REFERENCES devices(device_id) ON DELETE CASCADE
      );
    `);

    await queryRunner.query(`
      CREATE INDEX idx_tado_zone_mapping_device ON tado_zone_mappings(device_id);
    `);

    // Add new command types to the enum
    await queryRunner.query(`
      ALTER TYPE commands_type_enum ADD VALUE IF NOT EXISTS 'tado_set_auto_adjust';
    `);
    await queryRunner.query(`
      ALTER TYPE commands_type_enum ADD VALUE IF NOT EXISTS 'tado_sync_mapping';
    `);
  }

  public async down(queryRunner: QueryRunner): Promise<void> {
    await queryRunner.query(`DROP INDEX IF EXISTS idx_tado_zone_mapping_device;`);
    await queryRunner.query(`DROP TABLE IF EXISTS tado_zone_mappings;`);
    // Note: PostgreSQL doesn't support removing enum values, so we don't remove them in down
  }
}
