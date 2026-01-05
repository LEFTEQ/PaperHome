import {
  Controller,
  Get,
  Post,
  Patch,
  Delete,
  Param,
  Query,
  Body,
} from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
  ApiQuery,
} from '@nestjs/swagger';
import { TadoService, CreateZoneMappingDto, UpdateZoneMappingDto } from './tado.service';
import { CommandsService } from '../commands/commands.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';
import { CommandType } from '../../entities/command.entity';
import { IsNumber, Min, Max, IsString, IsBoolean, IsOptional } from 'class-validator';
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

// DTOs for zone mapping
class CreateZoneMappingRequestDto implements CreateZoneMappingDto {
  @ApiProperty({ description: 'Tado zone ID' })
  @IsNumber()
  tadoZoneId: number;

  @ApiProperty({ description: 'Tado zone name for display' })
  @IsString()
  tadoZoneName: string;

  @ApiProperty({
    description: 'Target room temperature in Celsius',
    minimum: 5,
    maximum: 30,
    required: false,
    default: 21.0,
  })
  @IsNumber()
  @Min(5)
  @Max(30)
  @IsOptional()
  targetTemperature?: number;

  @ApiProperty({
    description: 'Enable auto-adjust mode',
    required: false,
    default: false,
  })
  @IsBoolean()
  @IsOptional()
  autoAdjustEnabled?: boolean;
}

class UpdateZoneMappingRequestDto implements UpdateZoneMappingDto {
  @ApiProperty({
    description: 'Target room temperature in Celsius',
    minimum: 5,
    maximum: 30,
    required: false,
  })
  @IsNumber()
  @Min(5)
  @Max(30)
  @IsOptional()
  targetTemperature?: number;

  @ApiProperty({
    description: 'Enable auto-adjust mode',
    required: false,
  })
  @IsBoolean()
  @IsOptional()
  autoAdjustEnabled?: boolean;

  @ApiProperty({
    description: 'Temperature hysteresis threshold',
    minimum: 0.1,
    maximum: 2.0,
    required: false,
  })
  @IsNumber()
  @Min(0.1)
  @Max(2.0)
  @IsOptional()
  hysteresis?: number;
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

  // ==================== Zone Mapping Endpoints ====================

  @Get(':deviceId/zone-mappings')
  @ApiOperation({ summary: 'Get zone mappings for a device' })
  @ApiResponse({ status: 200, description: 'Zone mappings for device' })
  async getZoneMappings(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string
  ) {
    return this.tadoService.getZoneMappings(user.id, deviceId);
  }

  @Post(':deviceId/zone-mappings')
  @ApiOperation({ summary: 'Create a zone mapping for a device' })
  @ApiResponse({ status: 201, description: 'Zone mapping created' })
  async createZoneMapping(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Body() dto: CreateZoneMappingRequestDto
  ) {
    const mapping = await this.tadoService.createZoneMapping(user.id, deviceId, dto);

    // Sync to device via MQTT command
    await this.commandsService.sendCommand(user.id, {
      deviceId,
      type: CommandType.TADO_SYNC_MAPPING,
      payload: {
        zoneId: mapping.tadoZoneId,
        zoneName: mapping.tadoZoneName,
        targetTemperature: mapping.targetTemperature,
        autoAdjustEnabled: mapping.autoAdjustEnabled,
        hysteresis: mapping.hysteresis,
      },
    });

    return mapping;
  }

  @Patch(':deviceId/zone-mappings/:mappingId')
  @ApiOperation({ summary: 'Update a zone mapping' })
  @ApiResponse({ status: 200, description: 'Zone mapping updated' })
  async updateZoneMapping(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Param('mappingId') mappingId: string,
    @Body() dto: UpdateZoneMappingRequestDto
  ) {
    const mapping = await this.tadoService.updateZoneMapping(user.id, mappingId, dto);

    // Sync to device via MQTT command
    await this.commandsService.sendCommand(user.id, {
      deviceId,
      type: CommandType.TADO_SYNC_MAPPING,
      payload: {
        zoneId: mapping.tadoZoneId,
        zoneName: mapping.tadoZoneName,
        targetTemperature: mapping.targetTemperature,
        autoAdjustEnabled: mapping.autoAdjustEnabled,
        hysteresis: mapping.hysteresis,
      },
    });

    return mapping;
  }

  @Delete(':deviceId/zone-mappings/:mappingId')
  @ApiOperation({ summary: 'Delete a zone mapping' })
  @ApiResponse({ status: 200, description: 'Zone mapping deleted' })
  async deleteZoneMapping(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Param('mappingId') mappingId: string
  ) {
    // Get mapping before deletion to sync removal to device
    const mappings = await this.tadoService.getZoneMappings(user.id, deviceId);
    const mapping = mappings.find((m) => m.id === mappingId);

    await this.tadoService.deleteZoneMapping(user.id, mappingId);

    // Sync removal to device (disable auto-adjust for this zone)
    if (mapping) {
      await this.commandsService.sendCommand(user.id, {
        deviceId,
        type: CommandType.TADO_SYNC_MAPPING,
        payload: {
          zoneId: mapping.tadoZoneId,
          zoneName: mapping.tadoZoneName,
          targetTemperature: 21.0,
          autoAdjustEnabled: false,
          hysteresis: 0.5,
        },
      });
    }

    return { success: true };
  }
}
