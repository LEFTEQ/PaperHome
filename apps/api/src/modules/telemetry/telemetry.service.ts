import { Injectable, NotFoundException } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository, LessThanOrEqual, MoreThanOrEqual } from 'typeorm';
import { Telemetry } from '../../entities/telemetry.entity';
import { DevicesService } from '../devices/devices.service';

interface HistoryQuery {
  from?: Date;
  to?: Date;
  limit: number;
}

interface AggregateQuery {
  from?: Date;
  to?: Date;
  bucket: string;
}

@Injectable()
export class TelemetryService {
  constructor(
    @InjectRepository(Telemetry)
    private telemetryRepository: Repository<Telemetry>,
    private devicesService: DevicesService
  ) {}

  async create(data: Partial<Telemetry>): Promise<Telemetry> {
    const telemetry = this.telemetryRepository.create({
      ...data,
      time: data.time || new Date(),
    });
    return this.telemetryRepository.save(telemetry);
  }

  async getLatestForUser(userId: string) {
    const devices = await this.devicesService.findAllByOwner(userId);
    const results = [];

    for (const device of devices) {
      const latest = await this.telemetryRepository.findOne({
        where: { deviceId: device.deviceId },
        order: { time: 'DESC' },
      });

      results.push({
        deviceId: device.deviceId,
        deviceName: device.name,
        isOnline: device.isOnline,
        time: latest?.time || null,
        // STCC4 sensor data
        co2: latest?.co2 || null,
        temperature: latest?.temperature || null,
        humidity: latest?.humidity || null,
        battery: latest?.battery || null,
        // BME688/BSEC2 sensor data
        iaq: latest?.iaq ?? null,
        iaqAccuracy: latest?.iaqAccuracy ?? null,
        pressure: latest?.pressure ?? null,
        bme688Temperature: latest?.bme688Temperature ?? null,
        bme688Humidity: latest?.bme688Humidity ?? null,
      });
    }

    return results;
  }

  async getHistory(userId: string, deviceId: string, query: HistoryQuery) {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    const where: any = { deviceId };
    if (query.from) {
      where.time = MoreThanOrEqual(query.from);
    }
    if (query.to) {
      where.time = { ...where.time, ...LessThanOrEqual(query.to) };
    }

    return this.telemetryRepository.find({
      where,
      order: { time: 'DESC' },
      take: query.limit,
    });
  }

  async getAggregates(userId: string, deviceId: string, query: AggregateQuery) {
    // Verify user owns device
    const devices = await this.devicesService.findAllByOwner(userId);
    const device = devices.find((d) => d.deviceId === deviceId);
    if (!device) {
      throw new NotFoundException('Device not found');
    }

    const from = query.from || new Date(Date.now() - 24 * 60 * 60 * 1000);
    const to = query.to || new Date();

    // Use TimescaleDB time_bucket function for aggregation
    const result = await this.telemetryRepository.query(
      `
      SELECT
        time_bucket($1, time) AS bucket,
        AVG(co2) AS avg_co2,
        AVG(temperature) AS avg_temperature,
        AVG(humidity) AS avg_humidity,
        MIN(co2) AS min_co2,
        MAX(co2) AS max_co2,
        MIN(temperature) AS min_temperature,
        MAX(temperature) AS max_temperature,
        AVG(iaq) AS avg_iaq,
        MIN(iaq) AS min_iaq,
        MAX(iaq) AS max_iaq,
        AVG(pressure) AS avg_pressure,
        MIN(pressure) AS min_pressure,
        MAX(pressure) AS max_pressure,
        AVG(bme688_temperature) AS avg_bme688_temperature,
        AVG(bme688_humidity) AS avg_bme688_humidity
      FROM telemetry
      WHERE device_id = $2 AND time >= $3 AND time <= $4
      GROUP BY bucket
      ORDER BY bucket ASC
      `,
      [query.bucket, deviceId, from, to]
    );

    return result.map((row: any) => ({
      time: row.bucket,
      // STCC4 aggregates
      avgCo2: row.avg_co2 ? parseFloat(row.avg_co2) : null,
      avgTemperature: row.avg_temperature
        ? parseFloat(row.avg_temperature)
        : null,
      avgHumidity: row.avg_humidity ? parseFloat(row.avg_humidity) : null,
      minCo2: row.min_co2 ? parseFloat(row.min_co2) : null,
      maxCo2: row.max_co2 ? parseFloat(row.max_co2) : null,
      minTemperature: row.min_temperature
        ? parseFloat(row.min_temperature)
        : null,
      maxTemperature: row.max_temperature
        ? parseFloat(row.max_temperature)
        : null,
      // BME688 aggregates
      avgIaq: row.avg_iaq ? parseFloat(row.avg_iaq) : null,
      minIaq: row.min_iaq ? parseFloat(row.min_iaq) : null,
      maxIaq: row.max_iaq ? parseFloat(row.max_iaq) : null,
      avgPressure: row.avg_pressure ? parseFloat(row.avg_pressure) : null,
      minPressure: row.min_pressure ? parseFloat(row.min_pressure) : null,
      maxPressure: row.max_pressure ? parseFloat(row.max_pressure) : null,
      avgBme688Temperature: row.avg_bme688_temperature
        ? parseFloat(row.avg_bme688_temperature)
        : null,
      avgBme688Humidity: row.avg_bme688_humidity
        ? parseFloat(row.avg_bme688_humidity)
        : null,
    }));
  }
}
