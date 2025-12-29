import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { HueController } from './hue.controller';
import { HueService } from './hue.service';
import { HueState } from '../../entities/hue-state.entity';
import { DevicesModule } from '../devices/devices.module';

@Module({
  imports: [TypeOrmModule.forFeature([HueState]), DevicesModule],
  controllers: [HueController],
  providers: [HueService],
  exports: [HueService],
})
export class HueModule {}
