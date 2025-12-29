import { Controller, Get, Param, Query } from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
  ApiQuery,
} from '@nestjs/swagger';
import { TadoService } from './tado.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';

@ApiTags('tado')
@ApiBearerAuth()
@Controller('tado')
export class TadoController {
  constructor(private readonly tadoService: TadoService) {}

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
}
