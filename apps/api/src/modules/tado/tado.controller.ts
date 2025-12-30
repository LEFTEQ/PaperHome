import { Controller, Get, Post, Param, Query, Body } from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
  ApiQuery,
} from '@nestjs/swagger';
import { TadoService } from './tado.service';
import { CommandsService } from '../commands/commands.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';
import { CommandType } from '../../entities/command.entity';
import { IsNumber, Min, Max } from 'class-validator';
import { ApiProperty } from '@nestjs/swagger';

// DTO for temperature control
class SetTemperatureDto {
  @ApiProperty({
    description: 'Target temperature in Celsius',
    minimum: 5,
    maximum: 30,
  })
  @IsNumber()
  @Min(5)
  @Max(30)
  temperature: number;
}

@ApiTags('tado')
@ApiBearerAuth()
@Controller('tado')
export class TadoController {
  constructor(
    private readonly tadoService: TadoService,
    private readonly commandsService: CommandsService
  ) {}

  @Get(':deviceId/rooms')
  @ApiOperation({ summary: 'Get latest Tado room states for a device' })
  @ApiResponse({ status: 200, description: 'Latest Tado room states' })
  async getRooms(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string
  ) {
    return this.tadoService.getLatestRoomStates(user.id, deviceId);
  }

  @Get(':deviceId/rooms/history')
  @ApiOperation({ summary: 'Get Tado room state history' })
  @ApiQuery({ name: 'from', required: false, type: String })
  @ApiQuery({ name: 'to', required: false, type: String })
  @ApiQuery({ name: 'roomId', required: false, type: String })
  @ApiResponse({ status: 200, description: 'Tado state history' })
  async getHistory(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Query('from') from?: string,
    @Query('to') to?: string,
    @Query('roomId') roomId?: string
  ) {
    return this.tadoService.getHistory(user.id, deviceId, {
      from: from ? new Date(from) : undefined,
      to: to ? new Date(to) : undefined,
      roomId,
    });
  }

  @Post(':deviceId/rooms/:roomId/temperature')
  @ApiOperation({ summary: 'Set target temperature for a Tado room' })
  @ApiResponse({ status: 201, description: 'Command sent' })
  async setTemperature(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Param('roomId') roomId: string,
    @Body() dto: SetTemperatureDto
  ) {
    return this.commandsService.sendCommand(user.id, {
      deviceId,
      type: CommandType.TADO_SET_TEMP,
      payload: {
        roomId,
        temperature: dto.temperature,
      },
    });
  }
}
