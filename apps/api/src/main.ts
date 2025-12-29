import { Logger, ValidationPipe } from '@nestjs/common';
import { NestFactory } from '@nestjs/core';
import {
  FastifyAdapter,
  NestFastifyApplication,
} from '@nestjs/platform-fastify';
import { DocumentBuilder, SwaggerModule } from '@nestjs/swagger';
import { AppModule } from './app/app.module';

async function bootstrap() {
  const app = await NestFactory.create<NestFastifyApplication>(
    AppModule,
    new FastifyAdapter({ logger: true })
  );

  app.enableCors({
    origin: process.env.FRONTEND_URL || 'http://localhost:4200',
    credentials: true,
    methods: ['GET', 'POST', 'PUT', 'PATCH', 'DELETE', 'OPTIONS'],
  });

  app.useGlobalPipes(
    new ValidationPipe({
      whitelist: true,
      forbidNonWhitelisted: true,
      transform: true,
      transformOptions: {
        enableImplicitConversion: true,
      },
    })
  );

  const globalPrefix = 'api/v1';
  app.setGlobalPrefix(globalPrefix);

  // Swagger/OpenAPI setup
  const config = new DocumentBuilder()
    .setTitle('PaperHome API')
    .setDescription('IoT Dashboard API for ESP32 Smart Home Controller')
    .setVersion('1.0')
    .addBearerAuth()
    .addTag('auth', 'Authentication endpoints')
    .addTag('devices', 'Device management')
    .addTag('telemetry', 'Sensor telemetry data')
    .addTag('commands', 'Device command control')
    .addTag('hue', 'Philips Hue state management')
    .addTag('tado', 'Tado thermostat state management')
    .build();

  const document = SwaggerModule.createDocument(app, config);
  SwaggerModule.setup('docs', app, document, {
    jsonDocumentUrl: 'docs-json',
  });

  const port = process.env.API_PORT || 3000;
  await app.listen(port, '0.0.0.0');

  Logger.log(
    `Application is running on: http://localhost:${port}/${globalPrefix}`
  );
  Logger.log(`Swagger docs available at: http://localhost:${port}/docs`);
}

bootstrap();
