import { Controller, Get, Post, Body, Param, Query } from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
  ApiQuery,
} from '@nestjs/swagger';
import { CommandsService } from './commands.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';
import { CreateCommandDto } from './dto/create-command.dto';

@ApiTags('commands')
@ApiBearerAuth()
@Controller('commands')
export class CommandsController {
  constructor(private readonly commandsService: CommandsService) {}

  @Post()
  @ApiOperation({ summary: 'Send a command to a device via MQTT' })
  @ApiResponse({ status: 201, description: 'Command sent' })
  async create(@CurrentUser() user: any, @Body() createDto: CreateCommandDto) {
    return this.commandsService.sendCommand(user.id, createDto);
  }

  @Get(':deviceId')
  @ApiOperation({ summary: 'List recent commands for a device' })
  @ApiQuery({ name: 'limit', required: false, type: Number })
  @ApiResponse({ status: 200, description: 'Command history' })
  async getHistory(
    @CurrentUser() user: any,
    @Param('deviceId') deviceId: string,
    @Query('limit') limit?: number
  ) {
    return this.commandsService.getHistory(user.id, deviceId, limit || 50);
  }
}
