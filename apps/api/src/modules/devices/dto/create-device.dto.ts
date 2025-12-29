import { IsString, Matches, MaxLength } from 'class-validator';
import { ApiProperty } from '@nestjs/swagger';

export class CreateDeviceDto {
  @ApiProperty({
    example: 'A0B1C2D3E4F5',
    description: 'Device MAC address without separators',
  })
  @IsString()
  @Matches(/^[A-F0-9]{12}$/i, {
    message: 'Device ID must be a 12-character hex string (MAC address)',
  })
  deviceId: string;

  @ApiProperty({ example: 'Living Room Controller' })
  @IsString()
  @MaxLength(100)
  name: string;
}
