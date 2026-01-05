import { Module, forwardRef } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { TadoController } from './tado.controller';
import { TadoService } from './tado.service';
import { TadoState } from '../../entities/tado-state.entity';
import { TadoZoneMapping } from '../../entities/tado-zone-mapping.entity';
import { DevicesModule } from '../devices/devices.module';
import { CommandsModule } from '../commands/commands.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([TadoState, TadoZoneMapping]),
    DevicesModule,
    forwardRef(() => CommandsModule),
  ],
  controllers: [TadoController],
  providers: [TadoService],
  exports: [TadoService],
})
export class TadoModule {}
