import { Controller, Get, Param, Query } from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
  ApiQuery,
} from '@nestjs/swagger';
import { TelemetryService } from './telemetry.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';

@ApiTags('telemetry')
@ApiBearerAuth()
@Controller('telemetry')
export class TelemetryController {
  constructor(private readonly telemetryService: TelemetryService) {}

  @Get('latest')
  @ApiOperation({ summary: 'Get latest readings for all user devices' })
  @ApiResponse({ status: 200, description: 'Latest telemetry for all devices' })
  async getLatest(@CurrentUser() user: any) {
    return this.telemetryService.getLatestForUser(user.id);
  }

  @Get(':deviceId')
  @ApiOperation({ summary: 'Get telemetry history for a device' })
  @ApiQuery({ name: 'from', required: false, type: String })
  @ApiQuery({ name: 'to', required: false, type: String })
  @ApiQuery({ name: 'limit', required: false, type: Number })
  @ApiResponse({ status: 200, description: 'Telemetry history' })
  async getHistory(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Query('from') from?: string,
    @Query('to') to?: string,
    @Query('limit') limit?: number
  ) {
    return this.telemetryService.getHistory(user.id, deviceId, {
      from: from ? new Date(from) : undefined,
      to: to ? new Date(to) : undefined,
      limit: limit || 1000,
    });
  }

  @Get(':deviceId/aggregates')
  @ApiOperation({ summary: 'Get aggregated data for charts' })
  @ApiQuery({ name: 'from', required: false, type: String })
  @ApiQuery({ name: 'to', required: false, type: String })
  @ApiQuery({ name: 'bucket', required: false, type: String })
  @ApiResponse({ status: 200, description: 'Aggregated telemetry' })
  async getAggregates(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Query('from') from?: string,
    @Query('to') to?: string,
    @Query('bucket') bucket?: string
  ) {
    return this.telemetryService.getAggregates(user.id, deviceId, {
      from: from ? new Date(from) : undefined,
      to: to ? new Date(to) : undefined,
      bucket: bucket || '1 hour',
    });
  }
}
