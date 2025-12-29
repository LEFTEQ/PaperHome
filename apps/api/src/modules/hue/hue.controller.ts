import { Controller, Get, Param, Query } from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
  ApiQuery,
} from '@nestjs/swagger';
import { HueService } from './hue.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';

@ApiTags('hue')
@ApiBearerAuth()
@Controller('hue')
export class HueController {
  constructor(private readonly hueService: HueService) {}

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
}
