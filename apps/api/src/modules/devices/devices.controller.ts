import {
  Controller,
  Get,
  Post,
  Patch,
  Delete,
  Body,
  Param,
} from '@nestjs/common';
import {
  ApiTags,
  ApiOperation,
  ApiResponse,
  ApiBearerAuth,
} from '@nestjs/swagger';
import { DevicesService } from './devices.service';
import { CurrentUser } from '../auth/decorators/current-user.decorator';
import { CreateDeviceDto, UpdateDeviceDto } from './dto';

@ApiTags('devices')
@ApiBearerAuth()
@Controller('devices')
export class DevicesController {
  constructor(private readonly devicesService: DevicesService) {}

  @Get()
  @ApiOperation({ summary: 'List all devices for current user (owned + unclaimed)' })
  @ApiResponse({ status: 200, description: 'List of devices' })
  async findAll(@CurrentUser() user: any) {
    return this.devicesService.findAllForUser(user.id);
  }

  @Get('unclaimed')
  @ApiOperation({ summary: 'List all unclaimed devices' })
  @ApiResponse({ status: 200, description: 'List of unclaimed devices' })
  async findUnclaimed() {
    return this.devicesService.findAllUnclaimed();
  }

  @Post(':id/claim')
  @ApiOperation({ summary: 'Claim an unclaimed device' })
  @ApiResponse({ status: 200, description: 'Device claimed' })
  @ApiResponse({ status: 404, description: 'Unclaimed device not found' })
  async claim(@CurrentUser() user: any, @Param('id') id: string) {
    return this.devicesService.claimDevice(user.id, id);
  }

  @Post()
  @ApiOperation({ summary: 'Register a new device' })
  @ApiResponse({ status: 201, description: 'Device registered' })
  @ApiResponse({ status: 409, description: 'Device already registered' })
  async create(@CurrentUser() user: any, @Body() createDto: CreateDeviceDto) {
    return this.devicesService.create(user.id, createDto);
  }

  @Get(':id')
  @ApiOperation({ summary: 'Get device by ID (owned or unclaimed)' })
  @ApiResponse({ status: 200, description: 'Device details' })
  @ApiResponse({ status: 404, description: 'Device not found' })
  async findOne(@CurrentUser() user: any, @Param('id') id: string) {
    return this.devicesService.findOneForUser(user.id, id);
  }

  @Patch(':id')
  @ApiOperation({ summary: 'Update device name' })
  @ApiResponse({ status: 200, description: 'Device updated' })
  async update(
    @CurrentUser() user: any,
    @Param('id') id: string,
    @Body() updateDto: UpdateDeviceDto
  ) {
    return this.devicesService.update(user.id, id, updateDto);
  }

  @Delete(':id')
  @ApiOperation({ summary: 'Remove device' })
  @ApiResponse({ status: 200, description: 'Device removed' })
  async remove(@CurrentUser() user: any, @Param('id') id: string) {
    await this.devicesService.remove(user.id, id);
    return { message: 'Device removed' };
  }
}
