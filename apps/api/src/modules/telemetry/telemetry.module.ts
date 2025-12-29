import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { TelemetryController } from './telemetry.controller';
import { TelemetryService } from './telemetry.service';
import { Telemetry } from '../../entities/telemetry.entity';
import { DevicesModule } from '../devices/devices.module';

@Module({
  imports: [TypeOrmModule.forFeature([Telemetry]), DevicesModule],
  controllers: [TelemetryController],
  providers: [TelemetryService],
  exports: [TelemetryService],
})
export class TelemetryModule {}
