import { IsString, IsEnum, IsObject, Matches } from 'class-validator';
import { ApiProperty } from '@nestjs/swagger';
import { CommandType } from '../../../entities/command.entity';

export class CreateCommandDto {
  @ApiProperty({ example: 'A0B1C2D3E4F5' })
  @IsString()
  @Matches(/^[A-F0-9]{12}$/i)
  deviceId: string;

  @ApiProperty({ enum: CommandType, example: CommandType.HUE_SET_ROOM })
  @IsEnum(CommandType)
  type: CommandType;

  @ApiProperty({
    example: { roomId: 'living-room', isOn: true, brightness: 80 },
  })
  @IsObject()
  payload: Record<string, unknown>;
}
