import { IsString, MaxLength, IsOptional } from 'class-validator';
import { ApiProperty } from '@nestjs/swagger';

export class UpdateDeviceDto {
  @ApiProperty({ example: 'Bedroom Controller', required: false })
  @IsOptional()
  @IsString()
  @MaxLength(100)
  name?: string;
}
