import { Module, forwardRef } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { CommandsController } from './commands.controller';
import { CommandsService } from './commands.service';
import { Command } from '../../entities/command.entity';
import { DevicesModule } from '../devices/devices.module';
import { MqttModule } from '../mqtt/mqtt.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([Command]),
    DevicesModule,
    forwardRef(() => MqttModule),
  ],
  controllers: [CommandsController],
  providers: [CommandsService],
  exports: [CommandsService],
})
export class CommandsModule {}
