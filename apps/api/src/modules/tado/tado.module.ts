import { Module } from '@nestjs/common';
import { TypeOrmModule } from '@nestjs/typeorm';
import { TadoController } from './tado.controller';
import { TadoService } from './tado.service';
import { TadoState } from '../../entities/tado-state.entity';
import { DevicesModule } from '../devices/devices.module';

@Module({
  imports: [TypeOrmModule.forFeature([TadoState]), DevicesModule],
  controllers: [TadoController],
  providers: [TadoService],
  exports: [TadoService],
})
export class TadoModule {}
