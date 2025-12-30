import { useState, useMemo, useEffect } from 'react';
import { useNavigate } from 'react-router-dom';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Wind,
  Thermometer,
  Droplets,
  Battery,
  Cpu,
  Settings2,
  RefreshCw,
} from 'lucide-react';
import { DeviceTabBar, DeviceTab } from '@/components/ui/device-tab-bar';
import { ClaimDeviceBanner } from '@/components/ui/claim-device-banner';
import { MetricDetailModal, MetricType } from '@/components/ui/metric-detail-modal';
import { GlassCard, BentoCard } from '@/components/ui/glass-card';
import { Sparkline } from '@/components/ui/sparkline';
import { HueWidget, HueRoom } from '@/components/widgets/hue-widget';
import { TadoWidget, TadoRoom } from '@/components/widgets/tado-widget';
import { Button } from '@/components/ui/button';
import { staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';
import {
  useDevices,
  useClaimDevice,
  useHueRooms,
  useHueToggle,
  useHueBrightness,
  useTadoRooms,
  useTadoSetTemperature,
  useTelemetryAggregates,
  useLatestTelemetry,
  useRealtimeUpdates,
} from '@/hooks';

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
  metric: 'co2' | 'temperature' | 'humidity'
): number[] {
  if (!aggregates) return [];

  const key =
    metric === 'co2'
      ? 'avgCo2'
      : metric === 'temperature'
        ? 'avgTemperature'
        : 'avgHumidity';

  return aggregates
    .map((a) => a[key])
    .filter((v): v is number => v !== null);
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

  // Extract chart data
  const co2Data = useMemo(() => extractChartData(aggregates, 'co2'), [aggregates]);
  const tempData = useMemo(() => extractChartData(aggregates, 'temperature'), [aggregates]);
  const humidityData = useMemo(() => extractChartData(aggregates, 'humidity'), [aggregates]);

  // Get latest values - prefer real-time WebSocket data over aggregates
  const realtimeData = latestTelemetry?.find(
    (t) => t.deviceId === activeDevice?.deviceId
  );
  const latestAggregate = aggregates?.[aggregates.length - 1];

  // Use real-time values when available, fall back to aggregates
  const co2 = realtimeData?.co2 ?? latestAggregate?.avgCo2 ?? null;
  const temperature = realtimeData?.temperature ?? latestAggregate?.avgTemperature ?? null;
  const humidity = realtimeData?.humidity ?? latestAggregate?.avgHumidity ?? null;
  const battery = realtimeData?.battery ?? latestAggregate?.avgBattery ?? null;

  // Calculate stats for modal
  const co2Stats = useMemo(() => {
    if (!co2Data.length) return { min: 0, max: 0, avg: 0 };
    return {
      min: Math.min(...co2Data),
      max: Math.max(...co2Data),
      avg: co2Data.reduce((a, b) => a + b, 0) / co2Data.length,
    };
  }, [co2Data]);

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

  const handleBrightnessChange = (roomId: string, brightness: number) => {
    if (!activeDevice?.deviceId) return;
    hueBrightnessMutation.mutate({
      deviceId: activeDevice.deviceId,
      roomId,
      brightness,
      isOn: true, // Turn on when adjusting brightness
    });
  };

  const handleTadoTargetChange = (roomId: string, target: number) => {
    if (!activeDevice?.deviceId) return;
    tadoTempMutation.mutate({
      deviceId: activeDevice.deviceId,
      roomId,
      temperature: target,
    });
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
      <div className="max-w-6xl mx-auto px-4 space-y-6">
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
                {activeDevice?.name || 'Device'}
              </h1>
              <p className="text-sm text-text-muted font-mono">
                {activeDevice?.deviceId || ''}
              </p>
            </div>
          </div>

          <div className="flex items-center gap-2">
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

        {/* Bento Grid - Weather App Style */}
        <motion.div variants={fadeInUp}>
          {activeDevice?.isOnline && (co2 !== null || temperature !== null) ? (
            <div className="bento-grid">
              {/* CO2 - Hero Card (2x2) */}
              {co2 !== null && (
                <BentoCard
                  bentoSize="2x2"
                  interactive
                  onClick={() => handleMetricClick('co2')}
                  className={cn('metric-card metric-card-co2 cursor-pointer')}
                >
                  <div className="flex items-center justify-between mb-2">
                    <div className="flex items-center gap-2">
                      <Wind className="h-4 w-4 text-text-muted" />
                      <span className="text-sm font-medium text-text-muted uppercase tracking-wider">
                        Air Quality
                      </span>
                    </div>
                    <span
                      className={cn(
                        'text-sm font-semibold px-2.5 py-0.5 rounded-full',
                        getCO2Status(co2) === 'success' && 'bg-success/20 text-success',
                        getCO2Status(co2) === 'normal' && 'bg-accent/20 text-accent',
                        getCO2Status(co2) === 'warning' && 'bg-warning/20 text-warning',
                        getCO2Status(co2) === 'critical' && 'bg-error/20 text-error'
                      )}
                    >
                      {getCO2Label(co2)}
                    </span>
                  </div>

                  <div className="flex-1 flex flex-col justify-center">
                    <div className="flex items-baseline gap-2">
                      <span className="metric-value-large">{Math.round(co2)}</span>
                      <span className="text-2xl text-text-muted font-mono">ppm</span>
                    </div>
                    <p className="text-sm text-text-subtle mt-2">
                      CO₂ concentration in your air
                    </p>
                  </div>

                  {/* Sparkline */}
                  {co2Data.length > 0 && (
                    <div className="mt-4 h-16">
                      <Sparkline
                        data={co2Data}
                        color="#00e5ff"
                        height={64}
                        showFill
                        strokeWidth={2}
                      />
                    </div>
                  )}
                </BentoCard>
              )}

              {/* Temperature (1x1) */}
              {temperature !== null && (
                <BentoCard
                  bentoSize="1x1"
                  interactive
                  onClick={() => handleMetricClick('temperature')}
                  className={cn('metric-card metric-card-temperature cursor-pointer')}
                >
                  <div className="flex items-center gap-2 mb-3">
                    <Thermometer className="h-4 w-4 text-text-muted" />
                    <span className="text-xs font-medium text-text-muted uppercase tracking-wider">
                      Temperature
                    </span>
                  </div>
                  <div className="flex-1 flex flex-col justify-center">
                    <div className="flex items-baseline gap-1">
                      <span className="metric-value-medium">
                        {temperature.toFixed(1)}
                      </span>
                      <span className="text-lg text-text-muted font-mono">°C</span>
                    </div>
                  </div>
                  {tempData.length > 0 && (
                    <div className="mt-2 h-8">
                      <Sparkline
                        data={tempData}
                        color="#f59e0b"
                        height={32}
                        showFill
                      />
                    </div>
                  )}
                </BentoCard>
              )}

              {/* Humidity (1x1) */}
              {humidity !== null && (
                <BentoCard
                  bentoSize="1x1"
                  interactive
                  onClick={() => handleMetricClick('humidity')}
                  className={cn('metric-card metric-card-humidity cursor-pointer')}
                >
                  <div className="flex items-center gap-2 mb-3">
                    <Droplets className="h-4 w-4 text-text-muted" />
                    <span className="text-xs font-medium text-text-muted uppercase tracking-wider">
                      Humidity
                    </span>
                  </div>
                  <div className="flex-1 flex flex-col justify-center">
                    <div className="flex items-baseline gap-1">
                      <span className="metric-value-medium">
                        {Math.round(humidity)}
                      </span>
                      <span className="text-lg text-text-muted font-mono">%</span>
                    </div>
                  </div>
                  {humidityData.length > 0 && (
                    <div className="mt-2 h-8">
                      <Sparkline
                        data={humidityData}
                        color="#8b5cf6"
                        height={32}
                        showFill
                      />
                    </div>
                  )}
                </BentoCard>
              )}

              {/* Battery (1x1) */}
              {battery !== null && (
                <BentoCard
                  bentoSize="1x1"
                  interactive
                  onClick={() => handleMetricClick('battery')}
                  className={cn('metric-card metric-card-battery cursor-pointer')}
                >
                  <div className="flex items-center gap-2 mb-3">
                    <Battery className="h-4 w-4 text-text-muted" />
                    <span className="text-xs font-medium text-text-muted uppercase tracking-wider">
                      Battery
                    </span>
                  </div>
                  <div className="flex-1 flex flex-col justify-center">
                    <div className="flex items-baseline gap-1">
                      <span className="metric-value-medium">
                        {Math.round(battery)}
                      </span>
                      <span className="text-lg text-text-muted font-mono">%</span>
                    </div>
                    {/* Battery bar */}
                    <div className="mt-3 h-2 bg-glass rounded-full overflow-hidden">
                      <motion.div
                        className={cn(
                          'h-full rounded-full',
                          battery > 60 && 'bg-success',
                          battery <= 60 && battery > 30 && 'bg-warning',
                          battery <= 30 && 'bg-error'
                        )}
                        initial={{ width: 0 }}
                        animate={{ width: `${battery}%` }}
                        transition={{ duration: 0.8, ease: 'easeOut' }}
                      />
                    </div>
                  </div>
                </BentoCard>
              )}

              {/* Hue Lights (2x1) - Show if available */}
              {hueRooms.length > 0 && (
                <BentoCard
                  bentoSize="2x1"
                  className={cn('metric-card metric-card-hue')}
                >
                  <div className="flex items-center gap-2 mb-4">
                    <span className="text-xs font-medium text-text-muted uppercase tracking-wider">
                      Hue Lights
                    </span>
                  </div>
                  <div className="grid grid-cols-2 gap-3">
                    {hueRooms.slice(0, 2).map((room) => (
                      <HueWidget
                        key={room.id}
                        room={room}
                        onToggle={(isOn) => handleHueToggle(room.id, isOn)}
                        onBrightnessChange={(b) => handleBrightnessChange(room.id, b)}
                        compact
                      />
                    ))}
                  </div>
                </BentoCard>
              )}

              {/* Tado Thermostat (2x2) - Show if available */}
              {tadoRooms.length > 0 && (
                <TadoWidget
                  room={tadoRooms[0]}
                  onTargetChange={(target) =>
                    handleTadoTargetChange(tadoRooms[0].id, target)
                  }
                  bentoSize="2x2"
                  variant="full"
                  className={cn('metric-card metric-card-tado')}
                />
              )}
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

        {/* Additional Hue Rooms */}
        {hueRooms.length > 2 && (
          <motion.div variants={fadeInUp}>
            <h2 className="text-lg font-semibold text-white mb-4">More Lights</h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
              {hueRooms.slice(2).map((room) => (
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

        {/* Additional Tado Rooms */}
        {tadoRooms.length > 1 && (
          <motion.div variants={fadeInUp}>
            <h2 className="text-lg font-semibold text-white mb-4">More Rooms</h2>
            <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
              {tadoRooms.slice(1).map((room) => (
                <TadoWidget
                  key={room.id}
                  room={room}
                  onTargetChange={(target) => handleTadoTargetChange(room.id, target)}
                  variant="compact"
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
          modalMetric === 'co2'
            ? co2 ?? 0
            : modalMetric === 'temperature'
              ? temperature ?? 0
              : modalMetric === 'humidity'
                ? humidity ?? 0
                : battery ?? 0
        }
        unit={
          modalMetric === 'co2'
            ? 'ppm'
            : modalMetric === 'temperature'
              ? '°C'
              : '%'
        }
        chartData={
          modalMetric === 'co2'
            ? co2Data
            : modalMetric === 'temperature'
              ? tempData
              : modalMetric === 'humidity'
                ? humidityData
                : []
        }
        min={modalMetric === 'co2' ? co2Stats.min : undefined}
        max={modalMetric === 'co2' ? co2Stats.max : undefined}
        avg={modalMetric === 'co2' ? co2Stats.avg : undefined}
      />
    </motion.div>
  );
}

export default DashboardPage;
