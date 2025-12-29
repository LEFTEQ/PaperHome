import { Module, forwardRef } from '@nestjs/common';
import { MqttService } from './mqtt.service';
import { DevicesModule } from '../devices/devices.module';
import { TelemetryModule } from '../telemetry/telemetry.module';
import { HueModule } from '../hue/hue.module';
import { TadoModule } from '../tado/tado.module';
import { CommandsModule } from '../commands/commands.module';

@Module({
  imports: [
    DevicesModule,
    TelemetryModule,
    HueModule,
    TadoModule,
    forwardRef(() => CommandsModule),
  ],
  providers: [MqttService],
  exports: [MqttService],
})
export class MqttModule {}
