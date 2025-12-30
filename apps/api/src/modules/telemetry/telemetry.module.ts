import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { ScheduleModule } from '@nestjs/schedule';
import { TelemetryController } from './telemetry.controller';
import { TelemetryService } from './telemetry.service';
import { AggregationService } from './aggregation.service';
import { Telemetry } from '../../entities/telemetry.entity';
import { TelemetryHourly } from '../../entities/telemetry-hourly.entity';
import { DevicesModule } from '../devices/devices.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([Telemetry, TelemetryHourly]),
    ScheduleModule.forRoot(),
    DevicesModule,
  ],
  controllers: [TelemetryController],
  providers: [TelemetryService, AggregationService],
  exports: [TelemetryService, AggregationService],
})
export class TelemetryModule {}
