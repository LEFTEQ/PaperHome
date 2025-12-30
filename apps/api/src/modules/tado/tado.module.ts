import { Module, forwardRef } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { TadoController } from './tado.controller';
import { TadoService } from './tado.service';
import { TadoState } from '../../entities/tado-state.entity';
import { DevicesModule } from '../devices/devices.module';
import { CommandsModule } from '../commands/commands.module';

@Module({
  imports: [
    TypeOrmModule.forFeature([TadoState]),
    DevicesModule,
    forwardRef(() => CommandsModule),
  ],
  controllers: [TadoController],
  providers: [TadoService],
  exports: [TadoService],
})
export class TadoModule {}
