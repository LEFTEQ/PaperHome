import { Injectable, Logger } from '@nestjs/common';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository, LessThan } from 'typeorm';
import { Cron, CronExpression } from '@nestjs/schedule';
import { Telemetry } from '../../entities/telemetry.entity';
import { TelemetryHourly } from '../../entities/telemetry-hourly.entity';

/**
 * Data Retention Policy:
 * - Raw data (telemetry table): 7 days
 * - Hourly aggregates (telemetry_hourly table): 90 days
 *
 * Cron job runs every hour to:
 * 1. Aggregate raw data older than 7 days into hourly buckets
 * 2. Delete raw data older than 7 days
 * 3. Delete hourly aggregates older than 90 days
 */

const DAYS_RAW_RETENTION = 7;
const DAYS_AGGREGATE_RETENTION = 90;

@Injectable()
export class AggregationService {
  private readonly logger = new Logger(AggregationService.name);

  constructor(
    @InjectRepository(Telemetry)
    private telemetryRepository: Repository<Telemetry>,
    @InjectRepository(TelemetryHourly)
    private telemetryHourlyRepository: Repository<TelemetryHourly>
  ) {}

  /**
   * Run aggregation every hour
   */
  @Cron(CronExpression.EVERY_HOUR)
  async runAggregation() {
    this.logger.log('Starting telemetry aggregation...');

    try {
      const aggregated = await this.aggregateOldData();
      const deletedRaw = await this.deleteOldRawData();
      const deletedAggregates = await this.deleteOldAggregates();

      this.logger.log(
        `Aggregation complete: ${aggregated} hourly buckets created, ${deletedRaw} raw records deleted, ${deletedAggregates} old aggregates deleted`
      );
    } catch (error) {
      this.logger.error('Aggregation failed:', error);
    }
  }

  /**
   * Aggregate raw data older than retention period into hourly buckets
   */
  private async aggregateOldData(): Promise<number> {
    const cutoffDate = new Date();
    cutoffDate.setDate(cutoffDate.getDate() - DAYS_RAW_RETENTION);

    // Get distinct device IDs with old data
    const devices = await this.telemetryRepository
      .createQueryBuilder('t')
      .select('DISTINCT t.device_id', 'deviceId')
      .where('t.time < :cutoff', { cutoff: cutoffDate })
      .getRawMany();

    let totalAggregated = 0;

    for (const { deviceId } of devices) {
      // Use TimescaleDB time_bucket to aggregate by hour
      const aggregates = await this.telemetryRepository.query(
        `
        SELECT
          time_bucket('1 hour', time) AS hour,
          AVG(co2) AS avg_co2,
          MIN(co2) AS min_co2,
          MAX(co2) AS max_co2,
          AVG(temperature) AS avg_temperature,
          MIN(temperature) AS min_temperature,
          MAX(temperature) AS max_temperature,
          AVG(humidity) AS avg_humidity,
          MIN(humidity) AS min_humidity,
          MAX(humidity) AS max_humidity,
          AVG(battery) AS avg_battery,
          COUNT(*) AS sample_count
        FROM telemetry
        WHERE device_id = $1 AND time < $2
        GROUP BY hour
        ORDER BY hour ASC
        `,
        [deviceId, cutoffDate]
      );

      for (const row of aggregates) {
        // Upsert hourly aggregate (update if exists, insert if not)
        await this.telemetryHourlyRepository
          .createQueryBuilder()
          .insert()
          .into(TelemetryHourly)
          .values({
            deviceId,
            hour: row.hour,
            avgCo2: row.avg_co2 ? parseFloat(row.avg_co2) : null,
            minCo2: row.min_co2 ? parseFloat(row.min_co2) : null,
            maxCo2: row.max_co2 ? parseFloat(row.max_co2) : null,
            avgTemperature: row.avg_temperature
              ? parseFloat(row.avg_temperature)
              : null,
            minTemperature: row.min_temperature
              ? parseFloat(row.min_temperature)
              : null,
            maxTemperature: row.max_temperature
              ? parseFloat(row.max_temperature)
              : null,
            avgHumidity: row.avg_humidity
              ? parseFloat(row.avg_humidity)
              : null,
            minHumidity: row.min_humidity
              ? parseFloat(row.min_humidity)
              : null,
            maxHumidity: row.max_humidity
              ? parseFloat(row.max_humidity)
              : null,
            avgBattery: row.avg_battery ? parseFloat(row.avg_battery) : null,
            sampleCount: parseInt(row.sample_count, 10),
          })
          .orUpdate(
            [
              'avg_co2',
              'min_co2',
              'max_co2',
              'avg_temperature',
              'min_temperature',
              'max_temperature',
              'avg_humidity',
              'min_humidity',
              'max_humidity',
              'avg_battery',
              'sample_count',
            ],
            ['device_id', 'hour']
          )
          .execute();

        totalAggregated++;
      }
    }

    return totalAggregated;
  }

  /**
   * Delete raw telemetry data older than retention period
   */
  private async deleteOldRawData(): Promise<number> {
    const cutoffDate = new Date();
    cutoffDate.setDate(cutoffDate.getDate() - DAYS_RAW_RETENTION);

    const result = await this.telemetryRepository.delete({
      time: LessThan(cutoffDate),
    });

    return result.affected || 0;
  }

  /**
   * Delete hourly aggregates older than aggregate retention period
   */
  private async deleteOldAggregates(): Promise<number> {
    const cutoffDate = new Date();
    cutoffDate.setDate(cutoffDate.getDate() - DAYS_AGGREGATE_RETENTION);

    const result = await this.telemetryHourlyRepository.delete({
      hour: LessThan(cutoffDate),
    });

    return result.affected || 0;
  }

  /**
   * Get combined history (raw + aggregated) for a device
   */
  async getCombinedHistory(
    deviceId: string,
    from: Date,
    to: Date
  ): Promise<any[]> {
    const rawCutoff = new Date();
    rawCutoff.setDate(rawCutoff.getDate() - DAYS_RAW_RETENTION);

    const results: any[] = [];

    // Get raw data (for recent data within retention period)
    if (to > rawCutoff) {
      const rawFrom = from > rawCutoff ? from : rawCutoff;
      const rawData = await this.telemetryRepository.find({
        where: {
          deviceId,
          time: {
            $gte: rawFrom,
            $lte: to,
          } as any,
        },
        order: { time: 'ASC' },
      });

      results.push(
        ...rawData.map((t) => ({
          time: t.time,
          co2: t.co2,
          temperature: t.temperature,
          humidity: t.humidity,
          battery: t.battery,
          isAggregate: false,
        }))
      );
    }

    // Get aggregated data (for older data)
    if (from < rawCutoff) {
      const aggTo = to < rawCutoff ? to : rawCutoff;
      const aggData = await this.telemetryHourlyRepository.find({
        where: {
          deviceId,
          hour: {
            $gte: from,
            $lt: aggTo,
          } as any,
        },
        order: { hour: 'ASC' },
      });

      // Insert aggregates at the beginning (they're older)
      results.unshift(
        ...aggData.map((t) => ({
          time: t.hour,
          co2: t.avgCo2,
          temperature: t.avgTemperature,
          humidity: t.avgHumidity,
          battery: t.avgBattery,
          isAggregate: true,
        }))
      );
    }

    return results;
  }
}
