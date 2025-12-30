import { useMemo } from 'react';
import { motion } from 'framer-motion';
import { Wind, Thermometer, Droplets, Plus, RefreshCw } from 'lucide-react';
import { GlassCard } from '@/components/ui/glass-card';
import { Button } from '@/components/ui/button';
import { DevicesGrid, Device } from '@/components/widgets/device-summary-card';
import { staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';
import { useDevices, useLatestTelemetry, useRealtimeUpdates } from '@/hooks';

// Transform API data to Device interface for DevicesGrid
function transformDevices(
  devices: ReturnType<typeof useDevices>['data'],
  telemetry: ReturnType<typeof useLatestTelemetry>['data']
): Device[] {
  if (!devices) return [];

  return devices.map((device) => {
    const deviceTelemetry = telemetry?.find((t) => t.deviceId === device.deviceId);

    return {
      id: device.id,
      name: device.name,
      isOnline: device.isOnline,
      lastSeen: device.lastSeenAt ? new Date(device.lastSeenAt) : undefined,
      metrics: deviceTelemetry
        ? {
            co2: deviceTelemetry.co2 ?? undefined,
            temperature: deviceTelemetry.temperature ?? undefined,
            humidity: deviceTelemetry.humidity ?? undefined,
            battery: deviceTelemetry.battery ?? undefined,
          }
        : undefined,
      firmwareVersion: device.firmwareVersion ?? undefined,
    };
  });
}

// Calculate averages from online devices
function calculateAverages(devices: Device[]) {
  const onlineDevices = devices.filter((d) => d.isOnline && d.metrics);

  if (onlineDevices.length === 0) {
    return { avgCO2: null, avgTemp: null, avgHumidity: null };
  }

  const co2Values = onlineDevices
    .map((d) => d.metrics?.co2)
    .filter((v): v is number => v !== undefined);
  const tempValues = onlineDevices
    .map((d) => d.metrics?.temperature)
    .filter((v): v is number => v !== undefined);
  const humidityValues = onlineDevices
    .map((d) => d.metrics?.humidity)
    .filter((v): v is number => v !== undefined);

  return {
    avgCO2: co2Values.length
      ? Math.round(co2Values.reduce((a, b) => a + b, 0) / co2Values.length)
      : null,
    avgTemp: tempValues.length
      ? tempValues.reduce((a, b) => a + b, 0) / tempValues.length
      : null,
    avgHumidity: humidityValues.length
      ? Math.round(humidityValues.reduce((a, b) => a + b, 0) / humidityValues.length)
      : null,
  };
}

export function DashboardPage() {
  // Enable real-time updates
  const { isConnected } = useRealtimeUpdates();

  // Fetch data
  const {
    data: devicesData,
    isLoading: devicesLoading,
    error: devicesError,
    refetch: refetchDevices,
  } = useDevices();
  const {
    data: telemetryData,
    isLoading: telemetryLoading,
    refetch: refetchTelemetry,
  } = useLatestTelemetry();

  // Transform data for display
  const devices = useMemo(
    () => transformDevices(devicesData, telemetryData),
    [devicesData, telemetryData]
  );

  const onlineDevices = devices.filter((d) => d.isOnline).length;
  const totalDevices = devices.length;
  const { avgCO2, avgTemp, avgHumidity } = useMemo(
    () => calculateAverages(devices),
    [devices]
  );

  const isLoading = devicesLoading || telemetryLoading;

  const handleRefresh = () => {
    refetchDevices();
    refetchTelemetry();
  };

  // Error state
  if (devicesError) {
    return (
      <motion.div
        variants={staggerContainer}
        initial="initial"
        animate="animate"
        className="space-y-6"
      >
        <motion.div variants={fadeInUp}>
          <GlassCard className="p-8 text-center">
            <p className="text-error mb-4">Failed to load devices</p>
            <Button variant="secondary" onClick={handleRefresh}>
              Try Again
            </Button>
          </GlassCard>
        </motion.div>
      </motion.div>
    );
  }

  return (
    <motion.div
      variants={staggerContainer}
      initial="initial"
      animate="animate"
      className="space-y-6"
    >
      {/* Header */}
      <motion.div
        className="flex items-center justify-between"
        variants={fadeInUp}
      >
        <div>
          <h1 className="text-2xl font-bold text-white">Dashboard</h1>
          <p className="text-text-muted mt-1">
            {isLoading ? (
              'Loading...'
            ) : (
              <>
                {onlineDevices} of {totalDevices} devices online
                {isConnected && (
                  <span className="ml-2 text-success text-xs">● Live</span>
                )}
              </>
            )}
          </p>
        </div>
        <div className="flex items-center gap-2">
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={handleRefresh}
            disabled={isLoading}
            aria-label="Refresh"
          >
            <RefreshCw
              className={cn('h-4 w-4', isLoading && 'animate-spin')}
            />
          </Button>
          <Button variant="primary" leftIcon={<Plus className="h-4 w-4" />}>
            Add Device
          </Button>
        </div>
      </motion.div>

      {/* Summary cards */}
      <motion.div
        className="grid grid-cols-1 md:grid-cols-3 gap-4"
        variants={fadeInUp}
      >
        {/* Average CO2 */}
        <GlassCard className="flex items-center gap-4">
          <div
            className={cn(
              'h-12 w-12 rounded-xl flex items-center justify-center',
              'bg-accent-subtle'
            )}
          >
            <Wind className="h-6 w-6 text-accent" />
          </div>
          <div>
            <p className="text-sm text-text-muted">Avg. CO₂</p>
            {isLoading ? (
              <div className="h-8 w-20 bg-glass-elevated rounded animate-pulse" />
            ) : avgCO2 !== null ? (
              <p className="text-2xl font-bold font-mono text-white">
                {avgCO2}
                <span className="text-sm font-normal text-text-muted ml-1">
                  ppm
                </span>
              </p>
            ) : (
              <p className="text-lg text-text-subtle">No data</p>
            )}
          </div>
        </GlassCard>

        {/* Average Temperature */}
        <GlassCard className="flex items-center gap-4">
          <div
            className={cn(
              'h-12 w-12 rounded-xl flex items-center justify-center',
              'bg-warning-bg'
            )}
          >
            <Thermometer className="h-6 w-6 text-warning" />
          </div>
          <div>
            <p className="text-sm text-text-muted">Avg. Temperature</p>
            {isLoading ? (
              <div className="h-8 w-20 bg-glass-elevated rounded animate-pulse" />
            ) : avgTemp !== null ? (
              <p className="text-2xl font-bold font-mono text-white">
                {avgTemp.toFixed(1)}
                <span className="text-sm font-normal text-text-muted ml-1">
                  °C
                </span>
              </p>
            ) : (
              <p className="text-lg text-text-subtle">No data</p>
            )}
          </div>
        </GlassCard>

        {/* Average Humidity */}
        <GlassCard className="flex items-center gap-4">
          <div
            className={cn(
              'h-12 w-12 rounded-xl flex items-center justify-center',
              'bg-info/10'
            )}
          >
            <Droplets className="h-6 w-6 text-info" />
          </div>
          <div>
            <p className="text-sm text-text-muted">Avg. Humidity</p>
            {isLoading ? (
              <div className="h-8 w-20 bg-glass-elevated rounded animate-pulse" />
            ) : avgHumidity !== null ? (
              <p className="text-2xl font-bold font-mono text-white">
                {avgHumidity}
                <span className="text-sm font-normal text-text-muted ml-1">
                  %
                </span>
              </p>
            ) : (
              <p className="text-lg text-text-subtle">No data</p>
            )}
          </div>
        </GlassCard>
      </motion.div>

      {/* Devices section */}
      <motion.div variants={fadeInUp}>
        <h2 className="text-lg font-semibold text-white mb-4">Your Devices</h2>
        {isLoading ? (
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-4">
            {[1, 2, 3].map((i) => (
              <GlassCard key={i} className="h-40 animate-pulse" />
            ))}
          </div>
        ) : (
          <DevicesGrid devices={devices} />
        )}
      </motion.div>
    </motion.div>
  );
}
