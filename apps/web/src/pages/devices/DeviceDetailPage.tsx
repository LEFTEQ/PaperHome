import { useMemo } from 'react';
import { useParams, useNavigate } from 'react-router-dom';
import { motion } from 'framer-motion';
import {
  ArrowLeft,
  Battery,
  Settings2,
  Cpu,
  RefreshCw,
} from 'lucide-react';
import { Button } from '@/components/ui/button';
import { Badge, StatusBadge } from '@/components/ui/badge';
import { GlassCard, BentoCard } from '@/components/ui/glass-card';
import {
  CO2StatCard,
  TemperatureStatCard,
  HumidityStatCard,
  BatteryStatCard,
} from '@/components/widgets/stat-card';
import { HueWidget, HueRoom } from '@/components/widgets/hue-widget';
import { TadoWidget, TadoRoom } from '@/components/widgets/tado-widget';
import { ZoneMappingSection } from '@/components/widgets/zone-mapping-section';
import { OfflineState } from '@/components/ui/empty-state';
import { staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';
import {
  useDevice,
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

export function DeviceDetailPage() {
  const { id } = useParams<{ id: string }>();
  const navigate = useNavigate();

  // Enable real-time updates
  useRealtimeUpdates();

  // Mutations
  const hueToggleMutation = useHueToggle();
  const hueBrightnessMutation = useHueBrightness();
  const tadoTempMutation = useTadoSetTemperature();

  // Fetch device data
  const {
    data: device,
    isLoading: deviceLoading,
    error: deviceError,
    refetch: refetchDevice,
  } = useDevice(id);

  // Real-time telemetry from WebSocket
  const { data: latestTelemetry } = useLatestTelemetry();

  // Fetch Hue/Tado rooms
  const { data: hueRoomsData, isLoading: hueLoading } = useHueRooms(
    device?.deviceId
  );
  const { data: tadoRoomsData, isLoading: tadoLoading } = useTadoRooms(
    device?.deviceId
  );

  // Fetch telemetry aggregates for charts (last 24 hours, hourly)
  const fromDate = useMemo(() => {
    const d = new Date();
    d.setHours(d.getHours() - 24);
    return d.toISOString();
  }, []);

  const { data: aggregates } = useTelemetryAggregates(device?.deviceId, {
    from: fromDate,
    bucket: '1 hour',
  });

  // Transform data
  const hueRooms = useMemo(
    () => transformHueRooms(hueRoomsData),
    [hueRoomsData]
  );
  const tadoRooms = useMemo(
    () => transformTadoRooms(tadoRoomsData),
    [tadoRoomsData]
  );

  // Extract chart data
  const co2Data = useMemo(
    () => extractChartData(aggregates, 'co2'),
    [aggregates]
  );
  const tempData = useMemo(
    () => extractChartData(aggregates, 'temperature'),
    [aggregates]
  );
  const humidityData = useMemo(
    () => extractChartData(aggregates, 'humidity'),
    [aggregates]
  );

  // Get latest values - prefer real-time WebSocket data over aggregates
  const realtimeData = latestTelemetry?.find(
    (t) => t.deviceId === device?.deviceId
  );
  const latestAggregate = aggregates?.[aggregates.length - 1];

  // Use real-time values when available, fall back to aggregates
  const co2 = realtimeData?.co2 ?? latestAggregate?.avgCo2 ?? null;
  const temperature = realtimeData?.temperature ?? latestAggregate?.avgTemperature ?? null;
  const humidity = realtimeData?.humidity ?? latestAggregate?.avgHumidity ?? null;
  const battery = realtimeData?.battery ?? latestAggregate?.avgBattery ?? null;

  const isLoading = deviceLoading;

  // Handlers
  const handleHueToggle = (roomId: string, isOn: boolean) => {
    if (!device?.deviceId) return;
    hueToggleMutation.mutate({
      deviceId: device.deviceId,
      roomId,
      isOn,
    });
  };

  const handleBrightnessChange = (roomId: string, brightness: number) => {
    if (!device?.deviceId) return;
    hueBrightnessMutation.mutate({
      deviceId: device.deviceId,
      roomId,
      brightness,
      isOn: true,
    });
  };

  const handleTadoTargetChange = (roomId: string, target: number) => {
    if (!device?.deviceId) return;
    tadoTempMutation.mutate({
      deviceId: device.deviceId,
      roomId,
      temperature: target,
    });
  };

  // Loading state
  if (isLoading) {
    return (
      <motion.div
        variants={staggerContainer}
        initial="initial"
        animate="animate"
        className="space-y-6"
      >
        <motion.div variants={fadeInUp}>
          <div className="flex items-center gap-4 mb-6">
            <Button
              variant="ghost"
              size="sm"
              onClick={() => navigate('/dashboard')}
              leftIcon={<ArrowLeft className="h-4 w-4" />}
            >
              Back
            </Button>
            <div className="h-8 w-48 bg-glass-elevated rounded animate-pulse" />
          </div>
        </motion.div>
        <motion.div variants={fadeInUp} className="bento-grid">
          {[1, 2, 3, 4, 5, 6].map((i) => (
            <GlassCard key={i} className="h-48 animate-pulse" />
          ))}
        </motion.div>
      </motion.div>
    );
  }

  // Error state
  if (deviceError || !device) {
    return (
      <motion.div
        variants={staggerContainer}
        initial="initial"
        animate="animate"
        className="space-y-6"
      >
        <motion.div variants={fadeInUp}>
          <div className="flex items-center gap-4 mb-6">
            <Button
              variant="ghost"
              size="sm"
              onClick={() => navigate('/dashboard')}
              leftIcon={<ArrowLeft className="h-4 w-4" />}
            >
              Back
            </Button>
          </div>
        </motion.div>
        <motion.div variants={fadeInUp}>
          <GlassCard className="p-8 text-center">
            <p className="text-error mb-4">Device not found</p>
            <Button variant="secondary" onClick={() => refetchDevice()}>
              Try Again
            </Button>
          </GlassCard>
        </motion.div>
      </motion.div>
    );
  }

  // Offline state
  if (!device.isOnline) {
    return (
      <motion.div
        variants={staggerContainer}
        initial="initial"
        animate="animate"
        className="space-y-6"
      >
        {/* Header */}
        <motion.div variants={fadeInUp}>
          <div className="flex items-center gap-4 mb-6">
            <Button
              variant="ghost"
              size="sm"
              onClick={() => navigate('/dashboard')}
              leftIcon={<ArrowLeft className="h-4 w-4" />}
            >
              Back
            </Button>
            <div>
              <h1 className="text-2xl font-bold text-white">{device.name}</h1>
              <p className="text-sm font-mono text-text-muted">
                {device.deviceId}
              </p>
            </div>
          </div>
        </motion.div>
        <OfflineState />
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
        <div className="flex items-center gap-4">
          <Button
            variant="ghost"
            size="sm"
            onClick={() => navigate('/dashboard')}
            leftIcon={<ArrowLeft className="h-4 w-4" />}
          >
            Back
          </Button>
          <div className="flex items-center gap-3">
            <div
              className={cn(
                'h-12 w-12 rounded-xl flex items-center justify-center',
                'bg-success-bg'
              )}
            >
              <Cpu className="h-6 w-6 text-online" />
            </div>
            <div>
              <div className="flex items-center gap-3">
                <h1 className="text-2xl font-bold text-white">{device.name}</h1>
                <StatusBadge status="online" />
              </div>
              <p className="text-sm font-mono text-text-muted">
                {device.deviceId}
              </p>
            </div>
          </div>
        </div>

        <div className="flex items-center gap-4">
          {battery !== null && (
            <div className="flex items-center gap-2 text-sm">
              <Battery className="h-4 w-4 text-online" />
              <span className="font-mono text-white">{Math.round(battery)}%</span>
            </div>
          )}
          {device.firmwareVersion && (
            <Badge variant="outline" size="sm">
              v{device.firmwareVersion}
            </Badge>
          )}
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => refetchDevice()}
            aria-label="Refresh"
          >
            <RefreshCw className="h-4 w-4" />
          </Button>
          <Button variant="secondary" size="icon-sm" aria-label="Settings">
            <Settings2 className="h-4 w-4" />
          </Button>
        </div>
      </motion.div>

      {/* Bento Grid */}
      <motion.div variants={fadeInUp} className="bento-grid">
        {/* CO2 - Large widget */}
        {co2 !== null && (
          <CO2StatCard
            value={co2}
            chartData={co2Data}
            bentoSize="2x2"
            trend={
              co2Data.length > 1 && co2Data[co2Data.length - 1] > co2Data[0]
                ? 'up'
                : 'down'
            }
          />
        )}

        {/* Temperature */}
        {temperature !== null && (
          <TemperatureStatCard
            value={temperature}
            chartData={tempData}
            bentoSize="1x1"
          />
        )}

        {/* Humidity */}
        {humidity !== null && (
          <HumidityStatCard
            value={humidity}
            chartData={humidityData}
            bentoSize="1x1"
          />
        )}

        {/* Battery */}
        {battery !== null && (
          <BatteryStatCard value={battery} bentoSize="1x1" />
        )}

        {/* Tado Widget - Large */}
        {tadoRooms.length > 0 && (
          <TadoWidget
            room={tadoRooms[0]}
            onTargetChange={(target) =>
              handleTadoTargetChange(tadoRooms[0].id, target)
            }
            bentoSize="2x2"
            variant="full"
          />
        )}

        {/* Empty state if no sensor data */}
        {co2 === null && temperature === null && humidity === null && (
          <BentoCard bentoSize="2x2">
            <div className="flex flex-col items-center justify-center h-full text-center">
              <Cpu className="h-12 w-12 text-text-subtle mb-4" />
              <h3 className="text-lg font-semibold text-white mb-2">
                Waiting for data
              </h3>
              <p className="text-sm text-text-muted max-w-xs">
                Sensor data will appear here once your device starts reporting.
              </p>
            </div>
          </BentoCard>
        )}
      </motion.div>

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

      {/* Additional Tado Rooms */}
      {tadoRooms.length > 1 && (
        <motion.div variants={fadeInUp}>
          <h2 className="text-lg font-semibold text-white mb-4">
            More Tado Rooms
          </h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
            {tadoRooms.slice(1).map((room) => (
              <TadoWidget
                key={room.id}
                room={room}
                onTargetChange={(target) =>
                  handleTadoTargetChange(room.id, target)
                }
                variant="compact"
              />
            ))}
          </div>
        </motion.div>
      )}

      {/* Zone Mapping Section - Configure auto-adjust */}
      {tadoRoomsData && tadoRoomsData.length > 0 && (
        <motion.div variants={fadeInUp}>
          <ZoneMappingSection
            deviceId={device.deviceId}
            deviceName={device.name}
            isDeviceOnline={device.isOnline}
            availableZones={tadoRoomsData}
            esp32Temp={temperature ?? undefined}
          />
        </motion.div>
      )}
    </motion.div>
  );
}
