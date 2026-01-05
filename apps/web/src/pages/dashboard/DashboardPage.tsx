import { useState, useMemo, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { motion, AnimatePresence } from 'framer-motion';
import { formatDistanceToNow } from 'date-fns';
import {
  Wind,
  Thermometer,
  Droplets,
  Battery,
  Cpu,
  Settings2,
  RefreshCw,
  Gauge,
  Activity,
} from 'lucide-react';
import { DeviceTabBar, DeviceTab } from '@/components/ui/device-tab-bar';
import { ClaimDeviceBanner } from '@/components/ui/claim-device-banner';
import { MetricDetailModal, MetricType } from '@/components/ui/metric-detail-modal';
import { GlassCard } from '@/components/ui/glass-card';
import { HueWidget, HueRoom } from '@/components/widgets/hue-widget';
import { TadoWidget, TadoRoom, TadoZoneMappingData } from '@/components/widgets/tado-widget';
import { SensorWidget, SensorStats } from '@/components/widgets/sensor-widget';
import { EditableText } from '@/components/ui/editable-text';
import { Button } from '@/components/ui/button';
import { staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';
import {
  useDevices,
  useClaimDevice,
  useUpdateDevice,
  useHueRooms,
  useHueToggle,
  useHueBrightness,
  useTadoRooms,
  useTadoSetTemperature,
  useTelemetryAggregates,
  useLatestTelemetry,
  useRealtimeUpdates,
  useDebouncedCallback,
} from '@/hooks';
import {
  useZoneMappings,
  useToggleAutoAdjust,
  useSetTargetTemperature,
} from '@/hooks/use-zone-mappings';

// Transform API Hue data to widget format
function transformHueRooms(
  apiRooms: ReturnType<typeof useHueRooms>['data']
): HueRoom[] {
  if (!apiRooms) return [];
  return apiRooms.map((room) => ({
    id: room.roomId,
    name: room.roomName,
    isOn: room.isOn,
    brightness: room.brightness,
  }));
}

// Transform API Tado data to widget format
function transformTadoRooms(
  apiRooms: ReturnType<typeof useTadoRooms>['data']
): TadoRoom[] {
  if (!apiRooms) return [];
  return apiRooms.map((room) => ({
    id: room.roomId,
    name: room.roomName,
    currentTemp: room.currentTemp,
    targetTemp: room.targetTemp,
    humidity: room.humidity,
    isHeating: room.isHeating,
    mode: room.isHeating ? ('heat' as const) : ('off' as const),
  }));
}

// Extract chart data from aggregates
function extractChartData(
  aggregates: ReturnType<typeof useTelemetryAggregates>['data'],
  metric: 'co2' | 'temperature' | 'humidity' | 'iaq' | 'pressure' | 'battery' | 'bme688Temperature' | 'bme688Humidity'
): number[] {
  if (!aggregates) return [];

  const keyMap: Record<string, keyof NonNullable<typeof aggregates>[0]> = {
    co2: 'avgCo2',
    temperature: 'avgTemperature',
    humidity: 'avgHumidity',
    iaq: 'avgIaq',
    pressure: 'avgPressure',
    battery: 'avgBattery',
    bme688Temperature: 'avgBme688Temperature',
    bme688Humidity: 'avgBme688Humidity',
  };

  const key = keyMap[metric];
  return aggregates
    .map((a) => a[key] as number | null | undefined)
    .filter((v): v is number => v !== null && v !== undefined);
}

// Calculate stats from chart data
function calculateStats(data: number[]): SensorStats | null {
  if (!data.length) return null;
  return {
    min: Math.min(...data),
    max: Math.max(...data),
    avg: data.reduce((a, b) => a + b, 0) / data.length,
  };
}

// Get CO2 status color
function getCO2Status(value: number): 'success' | 'normal' | 'warning' | 'critical' {
  if (value < 600) return 'success';
  if (value < 1000) return 'normal';
  if (value < 1500) return 'warning';
  return 'critical';
}

// Get CO2 status label
function getCO2Label(value: number): string {
  if (value < 600) return 'Excellent';
  if (value < 1000) return 'Good';
  if (value < 1500) return 'Moderate';
  return 'Poor';
}

export function DashboardPage() {
  const navigate = useNavigate();

  // Enable real-time updates
  useRealtimeUpdates();

  // Fetch all devices (owned + unclaimed)
  const {
    data: devicesData,
    isLoading: devicesLoading,
    error: devicesError,
    refetch: refetchDevices,
  } = useDevices();

  // Mutations
  const claimMutation = useClaimDevice();
  const updateDeviceMutation = useUpdateDevice();
  const hueToggleMutation = useHueToggle();
  const hueBrightnessMutation = useHueBrightness();
  const tadoTempMutation = useTadoSetTemperature();

  // Real-time telemetry from WebSocket
  const { data: latestTelemetry } = useLatestTelemetry();

  // Active device state - auto-select first device
  const [activeDeviceId, setActiveDeviceId] = useState<string | null>(null);

  // Modal state
  const [modalOpen, setModalOpen] = useState(false);
  const [modalMetric, setModalMetric] = useState<MetricType>('co2');

  // Transform devices for tab bar
  const deviceTabs: DeviceTab[] = useMemo(() => {
    if (!devicesData) return [];
    return devicesData.map((device) => ({
      id: device.id,
      name: device.name,
      isOnline: device.isOnline,
      isUnclaimed: device.ownerId === null,
    }));
  }, [devicesData]);

  // Auto-select first device when devices load
  useEffect(() => {
    if (deviceTabs.length > 0 && !activeDeviceId) {
      // Prefer first online device, otherwise first device
      const onlineDevice = deviceTabs.find((d) => d.isOnline);
      setActiveDeviceId(onlineDevice?.id || deviceTabs[0].id);
    }
  }, [deviceTabs, activeDeviceId]);

  // Fetch active device details
  const activeDevice = useMemo(() => {
    return devicesData?.find((d) => d.id === activeDeviceId);
  }, [devicesData, activeDeviceId]);

  // Fetch Hue/Tado rooms for active device
  const { data: hueRoomsData } = useHueRooms(activeDevice?.deviceId);
  const { data: tadoRoomsData } = useTadoRooms(activeDevice?.deviceId);

  // Fetch zone mappings for auto-adjust feature
  const { data: zoneMappings = [] } = useZoneMappings(activeDevice?.deviceId);
  const toggleAutoAdjust = useToggleAutoAdjust();
  const setTargetTemperature = useSetTargetTemperature();

  // Fetch telemetry aggregates (last 24 hours, hourly)
  const fromDate = useMemo(() => {
    const d = new Date();
    d.setHours(d.getHours() - 24);
    return d.toISOString();
  }, []);

  const { data: aggregates } = useTelemetryAggregates(activeDevice?.deviceId, {
    from: fromDate,
    bucket: '1 hour',
  });

  // Transform data
  const hueRooms = useMemo(() => transformHueRooms(hueRoomsData), [hueRoomsData]);
  const tadoRooms = useMemo(() => transformTadoRooms(tadoRoomsData), [tadoRoomsData]);

  // Extract chart data for all metrics
  const co2Data = useMemo(() => extractChartData(aggregates, 'co2'), [aggregates]);
  const tempData = useMemo(() => extractChartData(aggregates, 'temperature'), [aggregates]);
  const humidityData = useMemo(() => extractChartData(aggregates, 'humidity'), [aggregates]);
  const iaqData = useMemo(() => extractChartData(aggregates, 'iaq'), [aggregates]);
  const pressureData = useMemo(() => extractChartData(aggregates, 'pressure'), [aggregates]);
  const batteryData = useMemo(() => extractChartData(aggregates, 'battery'), [aggregates]);
  const bme688TempData = useMemo(() => extractChartData(aggregates, 'bme688Temperature'), [aggregates]);
  const bme688HumidityData = useMemo(() => extractChartData(aggregates, 'bme688Humidity'), [aggregates]);

  // Get latest values - prefer real-time WebSocket data over aggregates
  const realtimeData = latestTelemetry?.find(
    (t) => t.deviceId === activeDevice?.deviceId
  );
  const latestAggregate = aggregates?.[aggregates.length - 1];

  // Use real-time values when available, fall back to aggregates
  // STCC4 sensor data
  const co2 = realtimeData?.co2 ?? latestAggregate?.avgCo2 ?? null;
  const temperature = realtimeData?.temperature ?? latestAggregate?.avgTemperature ?? null;
  const humidity = realtimeData?.humidity ?? latestAggregate?.avgHumidity ?? null;
  const battery = realtimeData?.battery ?? latestAggregate?.avgBattery ?? null;

  // BME688/BSEC2 sensor data
  const iaq = realtimeData?.iaq ?? latestAggregate?.avgIaq ?? null;
  const iaqAccuracy = realtimeData?.iaqAccuracy ?? 0;
  const pressure = realtimeData?.pressure ?? latestAggregate?.avgPressure ?? null;
  const bme688Temperature = realtimeData?.bme688Temperature ?? latestAggregate?.avgBme688Temperature ?? null;
  const bme688Humidity = realtimeData?.bme688Humidity ?? latestAggregate?.avgBme688Humidity ?? null;

  // Calculate stats for all metrics
  const co2Stats = useMemo(() => calculateStats(co2Data), [co2Data]);
  const tempStats = useMemo(() => calculateStats(tempData), [tempData]);
  const humidityStats = useMemo(() => calculateStats(humidityData), [humidityData]);
  const iaqStats = useMemo(() => calculateStats(iaqData), [iaqData]);
  const pressureStats = useMemo(() => calculateStats(pressureData), [pressureData]);
  const batteryStats = useMemo(() => calculateStats(batteryData), [batteryData]);
  const bme688TempStats = useMemo(() => calculateStats(bme688TempData), [bme688TempData]);
  const bme688HumidityStats = useMemo(() => calculateStats(bme688HumidityData), [bme688HumidityData]);

  // Calculate last updated timestamp
  const lastUpdated = useMemo(() => {
    const time = realtimeData?.time;
    if (!time) return null;
    return new Date(time);
  }, [realtimeData?.time]);

  // Check if active device is unclaimed
  const isUnclaimed = activeDevice?.ownerId === null;

  // Handle claim
  const handleClaim = async () => {
    if (!activeDeviceId) return;
    await claimMutation.mutateAsync(activeDeviceId);
    refetchDevices();
  };

  // Handle metric card click
  const handleMetricClick = (metric: MetricType) => {
    setModalMetric(metric);
    setModalOpen(true);
  };

  // Handle Hue/Tado interactions
  const handleHueToggle = (roomId: string, isOn: boolean) => {
    if (!activeDevice?.deviceId) return;
    hueToggleMutation.mutate({
      deviceId: activeDevice.deviceId,
      roomId,
      isOn,
    });
  };

  // Debounced brightness handler to avoid 503 errors from too many requests
  const handleBrightnessChange = useDebouncedCallback(
    (roomId: string, brightness: number) => {
      if (!activeDevice?.deviceId) return;
      hueBrightnessMutation.mutate({
        deviceId: activeDevice.deviceId,
        roomId,
        brightness,
        isOn: true, // Turn on when adjusting brightness
      });
    },
    200
  );

  const handleTadoTargetChange = (roomId: string, target: number) => {
    if (!activeDevice?.deviceId) return;
    tadoTempMutation.mutate({
      deviceId: activeDevice.deviceId,
      roomId,
      temperature: target,
    });
  };

  // Get zone mapping for a Tado room
  const getZoneMappingForRoom = (roomId: string): TadoZoneMappingData | undefined => {
    const zoneId = parseInt(roomId, 10);
    const mapping = zoneMappings.find((m) => m.tadoZoneId === zoneId);
    if (!mapping) return undefined;
    return {
      id: mapping.id,
      tadoZoneId: mapping.tadoZoneId,
      tadoZoneName: mapping.tadoZoneName,
      targetTemperature: mapping.targetTemperature,
      autoAdjustEnabled: mapping.autoAdjustEnabled,
      hysteresis: mapping.hysteresis,
    };
  };

  // Handle auto-adjust toggle via zone mapping
  const handleAutoAdjustToggle = (roomId: string, enabled: boolean) => {
    if (!activeDevice?.deviceId) return;
    const zoneId = parseInt(roomId, 10);
    const mapping = zoneMappings.find((m) => m.tadoZoneId === zoneId);
    if (!mapping) return;

    toggleAutoAdjust.mutate({
      deviceId: activeDevice.deviceId,
      mappingId: mapping.id,
      enabled,
      targetTemperature: mapping.targetTemperature,
    });
  };

  // Handle target temperature change via zone mapping (for auto-adjust)
  const handleZoneMappingTargetChange = (roomId: string, target: number) => {
    if (!activeDevice?.deviceId) return;
    const zoneId = parseInt(roomId, 10);
    const mapping = zoneMappings.find((m) => m.tadoZoneId === zoneId);

    if (mapping) {
      // Update via zone mapping (for auto-adjust mode)
      setTargetTemperature.mutate({
        deviceId: activeDevice.deviceId,
        mappingId: mapping.id,
        targetTemperature: target,
      });
    } else {
      // Direct Tado control (no zone mapping)
      handleTadoTargetChange(roomId, target);
    }
  };

  // Loading state
  if (devicesLoading) {
    return (
      <div className="min-h-[calc(100vh-4rem)] flex items-center justify-center">
        <motion.div
          initial={{ opacity: 0, scale: 0.9 }}
          animate={{ opacity: 1, scale: 1 }}
          className="flex flex-col items-center gap-4"
        >
          <div className="h-12 w-12 rounded-2xl bg-accent/20 flex items-center justify-center animate-pulse">
            <Cpu className="h-6 w-6 text-accent" />
          </div>
          <p className="text-text-muted text-sm">Loading devices...</p>
        </motion.div>
      </div>
    );
  }

  // Error state
  if (devicesError) {
    return (
      <div className="min-h-[calc(100vh-4rem)] flex items-center justify-center p-4">
        <GlassCard className="max-w-md text-center p-8">
          <p className="text-error mb-4">Failed to load devices</p>
          <Button variant="secondary" onClick={() => refetchDevices()}>
            Try Again
          </Button>
        </GlassCard>
      </div>
    );
  }

  // No devices state
  if (deviceTabs.length === 0) {
    return (
      <div className="min-h-[calc(100vh-4rem)] flex items-center justify-center p-4">
        <GlassCard className="max-w-md text-center p-8">
          <Cpu className="h-16 w-16 mx-auto text-text-subtle mb-4" />
          <h2 className="text-xl font-semibold text-white mb-2">No devices yet</h2>
          <p className="text-text-muted mb-6">
            Connect your first ESP32 device to start monitoring your home.
          </p>
          <Button variant="primary" onClick={() => navigate('/settings')}>
            Setup Guide
          </Button>
        </GlassCard>
      </div>
    );
  }

  return (
    <motion.div
      variants={staggerContainer}
      initial="initial"
      animate="animate"
      className="min-h-[calc(100vh-4rem)] py-8"
    >
      {/* Centered container */}
      <div className="max-w-6xl mx-auto space-y-6">
        {/* Device Tab Bar */}
        <motion.div variants={fadeInUp}>
          <DeviceTabBar
            devices={deviceTabs}
            activeDeviceId={activeDeviceId}
            onDeviceSelect={setActiveDeviceId}
          />
        </motion.div>

        {/* Claim Banner for unclaimed devices */}
        <AnimatePresence>
          {isUnclaimed && activeDevice && (
            <motion.div variants={fadeInUp}>
              <ClaimDeviceBanner
                deviceName={activeDevice.name}
                deviceId={activeDevice.deviceId}
                onClaim={handleClaim}
              />
            </motion.div>
          )}
        </AnimatePresence>

        {/* Device Header */}
        <motion.div
          variants={fadeInUp}
          className="flex items-center justify-between"
        >
          <div className="flex items-center gap-3">
            <div
              className={cn(
                'h-12 w-12 rounded-xl flex items-center justify-center',
                activeDevice?.isOnline ? 'bg-success-bg' : 'bg-glass'
              )}
            >
              <Cpu
                className={cn(
                  'h-6 w-6',
                  activeDevice?.isOnline ? 'text-online' : 'text-text-subtle'
                )}
              />
            </div>
            <div>
              <h1 className="text-2xl font-bold text-white">
                {activeDeviceId ? (
                  <EditableText
                    value={activeDevice?.name || 'Device'}
                    onSave={async (name) => {
                      await updateDeviceMutation.mutateAsync({
                        id: activeDeviceId,
                        data: { name },
                      });
                    }}
                    maxLength={100}
                    placeholder="Device name"
                  />
                ) : (
                  'Device'
                )}
              </h1>
              <p className="text-sm text-text-muted font-mono">
                {activeDevice?.deviceId || ''}
              </p>
            </div>
          </div>

          <div className="flex items-center gap-3">
            {lastUpdated && (
              <span className="text-xs text-text-subtle font-mono hidden sm:block">
                Updated {formatDistanceToNow(lastUpdated, { addSuffix: true })}
              </span>
            )}
            <Button
              variant="ghost"
              size="icon-sm"
              onClick={() => refetchDevices()}
              aria-label="Refresh"
            >
              <RefreshCw className="h-4 w-4" />
            </Button>
            <Button
              variant="secondary"
              size="icon-sm"
              onClick={() => navigate('/settings')}
              aria-label="Settings"
            >
              <Settings2 className="h-4 w-4" />
            </Button>
          </div>
        </motion.div>

        {/* Sensor Grid - Fixed 4×3 Layout */}
        <motion.div variants={fadeInUp}>
          {activeDevice?.isOnline && (co2 !== null || temperature !== null || iaq !== null) ? (
            <div className="sensor-grid">
              {/* CO2 Hero (2×2) */}
              <SensorWidget
                className="sensor-grid-co2"
                metricType="co2"
                sensorSource="STCC4"
                value={co2}
                unit="ppm"
                chartData={co2Data}
                stats={co2Stats}
                color="#00e5ff"
                icon={Wind}
                bentoSize="2x2"
                onClick={() => handleMetricClick('co2')}
              />

              {/* Temperature STCC4 (1×1) */}
              <SensorWidget
                className="sensor-grid-temp-stcc4"
                metricType="temperature"
                sensorSource="STCC4"
                value={temperature}
                unit="°C"
                chartData={tempData}
                stats={tempStats}
                color="#f59e0b"
                icon={Thermometer}
                bentoSize="1x1"
                onClick={() => handleMetricClick('temperature')}
              />

              {/* Temperature BME688 (1×1) */}
              <SensorWidget
                className="sensor-grid-temp-bme688"
                metricType="temperature"
                sensorSource="BME688"
                value={bme688Temperature}
                unit="°C"
                chartData={bme688TempData}
                stats={bme688TempStats}
                color="#f59e0b"
                icon={Thermometer}
                bentoSize="1x1"
                onClick={() => handleMetricClick('temperature')}
              />

              {/* Humidity STCC4 (1×1) */}
              <SensorWidget
                className="sensor-grid-humid-stcc4"
                metricType="humidity"
                sensorSource="STCC4"
                value={humidity}
                unit="%"
                chartData={humidityData}
                stats={humidityStats}
                color="#8b5cf6"
                icon={Droplets}
                bentoSize="1x1"
                onClick={() => handleMetricClick('humidity')}
              />

              {/* Humidity BME688 (1×1) */}
              <SensorWidget
                className="sensor-grid-humid-bme688"
                metricType="humidity"
                sensorSource="BME688"
                value={bme688Humidity}
                unit="%"
                chartData={bme688HumidityData}
                stats={bme688HumidityStats}
                color="#8b5cf6"
                icon={Droplets}
                bentoSize="1x1"
                onClick={() => handleMetricClick('humidity')}
              />

              {/* Pressure (1×2 horizontal) */}
              <SensorWidget
                className="sensor-grid-pressure"
                metricType="pressure"
                sensorSource="BME688"
                value={pressure}
                unit="hPa"
                chartData={pressureData}
                stats={pressureStats}
                color="#a855f7"
                icon={Gauge}
                bentoSize="2x1"
                onClick={() => handleMetricClick('pressure')}
              />

              {/* Battery (1×1) */}
              <SensorWidget
                className="sensor-grid-battery"
                metricType="battery"
                value={battery}
                unit="%"
                chartData={batteryData}
                stats={batteryStats}
                color="#22c55e"
                icon={Battery}
                bentoSize="1x1"
                onClick={() => handleMetricClick('battery')}
              />

              {/* IAQ (1×1) */}
              <SensorWidget
                className="sensor-grid-iaq"
                metricType="iaq"
                sensorSource="BME688"
                value={iaq}
                unit="IAQ"
                chartData={iaqData}
                stats={iaqStats}
                color="#14b8a6"
                icon={Activity}
                bentoSize="1x1"
                iaqAccuracy={iaqAccuracy}
                onClick={() => handleMetricClick('iaq')}
              />
            </div>
          ) : (
            // Offline or no data state
            <GlassCard className="p-12 text-center">
              <Cpu className="h-16 w-16 mx-auto text-text-subtle mb-4" />
              <h3 className="text-xl font-semibold text-white mb-2">
                {activeDevice?.isOnline ? 'Waiting for data' : 'Device offline'}
              </h3>
              <p className="text-text-muted max-w-sm mx-auto">
                {activeDevice?.isOnline
                  ? 'Sensor data will appear here once your device starts reporting.'
                  : 'This device is currently offline. Data will appear when it reconnects.'}
              </p>
            </GlassCard>
          )}
        </motion.div>

        {/* Climate Control Section - Interactive Tado widgets */}
        {tadoRooms.length > 0 && (
          <motion.div variants={fadeInUp}>
            <h2 className="text-lg font-semibold text-white mb-4">Climate Control</h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
              {tadoRooms.map((room) => {
                const zoneMapping = getZoneMappingForRoom(room.id);
                return (
                  <TadoWidget
                    key={room.id}
                    room={room}
                    zoneMapping={zoneMapping}
                    esp32Temp={temperature ?? undefined}
                    isDeviceOnline={activeDevice?.isOnline}
                    onTargetChange={(target) => handleZoneMappingTargetChange(room.id, target)}
                    onAutoAdjustToggle={
                      zoneMapping
                        ? (enabled) => handleAutoAdjustToggle(room.id, enabled)
                        : undefined
                    }
                    variant={zoneMapping ? 'interactive' : 'compact'}
                  />
                );
              })}
            </div>
          </motion.div>
        )}

        {/* Lights Section */}
        {hueRooms.length > 0 && (
          <motion.div variants={fadeInUp}>
            <h2 className="text-lg font-semibold text-white mb-4">Lights</h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
              {hueRooms.map((room) => (
                <HueWidget
                  key={room.id}
                  room={room}
                  onToggle={(isOn) => handleHueToggle(room.id, isOn)}
                  onBrightnessChange={(b) => handleBrightnessChange(room.id, b)}
                />
              ))}
            </div>
          </motion.div>
        )}
      </div>

      {/* Metric Detail Modal */}
      <MetricDetailModal
        isOpen={modalOpen}
        onClose={() => setModalOpen(false)}
        metricType={modalMetric}
        currentValue={
          modalMetric === 'co2' ? co2 ?? 0
          : modalMetric === 'temperature' ? temperature ?? 0
          : modalMetric === 'humidity' ? humidity ?? 0
          : modalMetric === 'iaq' ? iaq ?? 0
          : modalMetric === 'pressure' ? pressure ?? 0
          : battery ?? 0
        }
        unit={
          modalMetric === 'co2' ? 'ppm'
          : modalMetric === 'temperature' ? '°C'
          : modalMetric === 'humidity' || modalMetric === 'battery' ? '%'
          : modalMetric === 'iaq' ? 'IAQ'
          : 'hPa'
        }
        chartData={
          modalMetric === 'co2' ? co2Data
          : modalMetric === 'temperature' ? tempData
          : modalMetric === 'humidity' ? humidityData
          : modalMetric === 'iaq' ? iaqData
          : modalMetric === 'pressure' ? pressureData
          : batteryData
        }
        min={
          modalMetric === 'co2' ? co2Stats?.min
          : modalMetric === 'temperature' ? tempStats?.min
          : modalMetric === 'humidity' ? humidityStats?.min
          : modalMetric === 'iaq' ? iaqStats?.min
          : modalMetric === 'pressure' ? pressureStats?.min
          : batteryStats?.min
        }
        max={
          modalMetric === 'co2' ? co2Stats?.max
          : modalMetric === 'temperature' ? tempStats?.max
          : modalMetric === 'humidity' ? humidityStats?.max
          : modalMetric === 'iaq' ? iaqStats?.max
          : modalMetric === 'pressure' ? pressureStats?.max
          : batteryStats?.max
        }
        avg={
          modalMetric === 'co2' ? co2Stats?.avg
          : modalMetric === 'temperature' ? tempStats?.avg
          : modalMetric === 'humidity' ? humidityStats?.avg
          : modalMetric === 'iaq' ? iaqStats?.avg
          : modalMetric === 'pressure' ? pressureStats?.avg
          : batteryStats?.avg
        }
      />
    </motion.div>
  );
}

export default DashboardPage;
