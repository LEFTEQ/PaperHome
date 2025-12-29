import { useParams, Link, useNavigate } from 'react-router-dom';
import { motion } from 'framer-motion';
import {
  ArrowLeft,
  Battery,
  Settings2,
  Cpu,
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
import { OfflineState } from '@/components/ui/empty-state';
import { staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';

// Mock data generators
const generateSparklineData = (
  hours: number,
  metric: 'co2' | 'temperature' | 'humidity'
) => {
  const data: number[] = [];
  for (let i = 0; i < hours; i++) {
    let value;
    if (metric === 'co2') {
      value = 500 + Math.random() * 300 + Math.sin(i / 4) * 100;
    } else if (metric === 'temperature') {
      value = 20 + Math.random() * 4 + Math.sin(i / 6) * 2;
    } else {
      value = 40 + Math.random() * 20 + Math.sin(i / 5) * 10;
    }
    data.push(Math.round(value * 10) / 10);
  }
  return data;
};

// Mock Hue rooms
const mockHueRooms: HueRoom[] = [
  { id: '1', name: 'Living Room', isOn: true, brightness: 80 },
  { id: '2', name: 'Kitchen', isOn: true, brightness: 100 },
  { id: '3', name: 'Bedroom', isOn: false, brightness: 0 },
  { id: '4', name: 'Bathroom', isOn: true, brightness: 60 },
];

// Mock Tado rooms
const mockTadoRooms: TadoRoom[] = [
  {
    id: '1',
    name: 'Living Room',
    currentTemp: 21.5,
    targetTemp: 22,
    humidity: 45,
    isHeating: true,
    mode: 'heat',
  },
  {
    id: '2',
    name: 'Bedroom',
    currentTemp: 19.8,
    targetTemp: 20,
    humidity: 52,
    isHeating: false,
    mode: 'heat',
  },
];

export function DeviceDetailPage() {
  const { id } = useParams();
  const navigate = useNavigate();

  // Mock device data - replace with API
  const device = {
    id,
    deviceId: 'A0B1C2D3E4F5',
    name: 'Living Room',
    isOnline: true,
    battery: 85,
    firmwareVersion: '1.2.0',
    co2: 650,
    temperature: 22.5,
    humidity: 45,
  };

  // Generate sparkline data
  const co2Data = generateSparklineData(24, 'co2');
  const tempData = generateSparklineData(24, 'temperature');
  const humidityData = generateSparklineData(24, 'humidity');

  // Handlers
  const handleHueToggle = (roomId: string, isOn: boolean) => {
    console.log(`Toggle room ${roomId} to ${isOn}`);
  };

  const handleBrightnessChange = (roomId: string, brightness: number) => {
    console.log(`Set room ${roomId} brightness to ${brightness}`);
  };

  const handleTadoTargetChange = (roomId: string, target: number) => {
    console.log(`Set room ${roomId} target to ${target}`);
  };

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
          <div className="flex items-center gap-2 text-sm">
            <Battery className="h-4 w-4 text-online" />
            <span className="font-mono text-white">{device.battery}%</span>
          </div>
          <Badge variant="outline" size="sm">
            v{device.firmwareVersion}
          </Badge>
          <Button variant="secondary" size="icon-sm" aria-label="Settings">
            <Settings2 className="h-4 w-4" />
          </Button>
        </div>
      </motion.div>

      {/* Bento Grid */}
      <motion.div variants={fadeInUp} className="bento-grid">
        {/* CO2 - Large widget */}
        <CO2StatCard
          value={device.co2}
          chartData={co2Data}
          bentoSize="2x2"
          trend="up"
          trendValue={5}
        />

        {/* Temperature */}
        <TemperatureStatCard
          value={device.temperature}
          chartData={tempData}
          bentoSize="1x1"
        />

        {/* Humidity */}
        <HumidityStatCard
          value={device.humidity}
          chartData={humidityData}
          bentoSize="1x1"
        />

        {/* Battery */}
        <BatteryStatCard value={device.battery} bentoSize="1x1" />

        {/* Hue Widget - spans 2 columns */}
        <BentoCard bentoSize="2x1">
          <h3 className="text-sm font-medium text-text-muted mb-4">
            Hue Lights
          </h3>
          <div className="grid grid-cols-2 gap-3">
            {mockHueRooms.slice(0, 2).map((room) => (
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

        {/* Tado Widget - Large */}
        <TadoWidget
          room={mockTadoRooms[0]}
          onTargetChange={(target) =>
            handleTadoTargetChange(mockTadoRooms[0].id, target)
          }
          bentoSize="2x2"
          variant="full"
        />
      </motion.div>

      {/* Additional Hue Rooms */}
      {mockHueRooms.length > 2 && (
        <motion.div variants={fadeInUp}>
          <h2 className="text-lg font-semibold text-white mb-4">
            More Hue Rooms
          </h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
            {mockHueRooms.slice(2).map((room) => (
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
      {mockTadoRooms.length > 1 && (
        <motion.div variants={fadeInUp}>
          <h2 className="text-lg font-semibold text-white mb-4">
            More Tado Rooms
          </h2>
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
            {mockTadoRooms.slice(1).map((room) => (
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
    </motion.div>
  );
}
