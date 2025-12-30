import { Module } from '@nestjs/common';
import { ConfigModule, ConfigService } from '@nestjs/config';
import { TypeOrmModule } from '@nestjs/typeorm';
import { APP_GUARD } from '@nestjs/core';
import { AppController } from './app.controller';
import { AppService } from './app.service';
import { dataSourceOptions } from '../database/data-source';
import { AuthModule } from '../modules/auth/auth.module';
import { DevicesModule } from '../modules/devices/devices.module';
import { TelemetryModule } from '../modules/telemetry/telemetry.module';
import { CommandsModule } from '../modules/commands/commands.module';
import { HueModule } from '../modules/hue/hue.module';
import { TadoModule } from '../modules/tado/tado.module';
import { MqttModule } from '../modules/mqtt/mqtt.module';
import { WebSocketModule } from '../modules/websocket/websocket.module';
import { JwtAuthGuard } from '../modules/auth/guards/jwt-auth.guard';

@Module({
  imports: [
    // Configuration
    ConfigModule.forRoot({
      isGlobal: true,
      envFilePath: '.env',
    }),

    // Database
    TypeOrmModule.forRootAsync({
      imports: [ConfigModule],
      useFactory: () => dataSourceOptions,
      inject: [ConfigService],
    }),

    // Feature Modules
    AuthModule,
    DevicesModule,
    TelemetryModule,
    CommandsModule,
    HueModule,
    TadoModule,
    MqttModule,
    WebSocketModule,
  ],
  controllers: [AppController],
  providers: [
    AppService,
    // Global JWT guard
    {
      provide: APP_GUARD,
      useClass: JwtAuthGuard,
    },
  ],
})
export class AppModule {}
