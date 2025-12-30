import { Controller, Get, Post, Param, Query, Body } from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
  ApiQuery,
} from '@nestjs/swagger';
import { HueService } from './hue.service';
import { CommandsService } from '../commands/commands.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';
import { CommandType } from '../../entities/command.entity';
import { IsBoolean, IsInt, Min, Max, IsOptional } from 'class-validator';
import { ApiProperty } from '@nestjs/swagger';

// DTOs for control endpoints
class ToggleRoomDto {
  @ApiProperty({ description: 'Whether to turn the room on or off' })
  @IsBoolean()
  isOn: boolean;
}

class SetBrightnessDto {
  @ApiProperty({ description: 'Brightness level (0-100)', minimum: 0, maximum: 100 })
  @IsInt()
  @Min(0)
  @Max(100)
  brightness: number;

  @ApiProperty({ description: 'Whether to turn on when setting brightness', required: false })
  @IsOptional()
  @IsBoolean()
  isOn?: boolean;
}

@ApiTags('hue')
@ApiBearerAuth()
@Controller('hue')
export class HueController {
  constructor(
    private readonly hueService: HueService,
    private readonly commandsService: CommandsService
  ) {}

  @Get(':deviceId/rooms')
  @ApiOperation({ summary: 'Get latest Hue room states for a device' })
  @ApiResponse({ status: 200, description: 'Latest Hue room states' })
  async getRooms(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string
  ) {
    return this.hueService.getLatestRoomStates(user.id, deviceId);
  }

  @Get(':deviceId/rooms/history')
  @ApiOperation({ summary: 'Get Hue room state history' })
  @ApiQuery({ name: 'from', required: false, type: String })
  @ApiQuery({ name: 'to', required: false, type: String })
  @ApiQuery({ name: 'roomId', required: false, type: String })
  @ApiResponse({ status: 200, description: 'Hue state history' })
  async getHistory(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Query('from') from?: string,
    @Query('to') to?: string,
    @Query('roomId') roomId?: string
  ) {
    return this.hueService.getHistory(user.id, deviceId, {
      from: from ? new Date(from) : undefined,
      to: to ? new Date(to) : undefined,
      roomId,
    });
  }

  @Post(':deviceId/rooms/:roomId/toggle')
  @ApiOperation({ summary: 'Toggle a Hue room on/off' })
  @ApiResponse({ status: 201, description: 'Command sent' })
  async toggleRoom(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Param('roomId') roomId: string,
    @Body() dto: ToggleRoomDto
  ) {
    return this.commandsService.sendCommand(user.id, {
      deviceId,
      type: CommandType.HUE_SET_ROOM,
      payload: {
        roomId,
        isOn: dto.isOn,
      },
    });
  }

  @Post(':deviceId/rooms/:roomId/brightness')
  @ApiOperation({ summary: 'Set brightness for a Hue room' })
  @ApiResponse({ status: 201, description: 'Command sent' })
  async setBrightness(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Param('roomId') roomId: string,
    @Body() dto: SetBrightnessDto
  ) {
    return this.commandsService.sendCommand(user.id, {
      deviceId,
      type: CommandType.HUE_SET_ROOM,
      payload: {
        roomId,
        brightness: dto.brightness,
        isOn: dto.isOn !== undefined ? dto.isOn : dto.brightness > 0,
      },
    });
  }
}
