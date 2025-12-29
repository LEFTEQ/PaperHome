import { motion } from 'framer-motion';
import { Wind, Thermometer, Droplets, Plus } from 'lucide-react';
import { GlassCard } from '@/components/ui/glass-card';
import { Button } from '@/components/ui/button';
import { DevicesGrid, Device } from '@/components/widgets/device-summary-card';
import { NoDevicesState } from '@/components/ui/empty-state';
import { staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';

// Mock data - will be replaced with API calls
const mockDevices: Device[] = [
  {
    id: '1',
    name: 'Living Room',
    isOnline: true,
    lastSeen: new Date(),
    metrics: {
      co2: 650,
      temperature: 22.5,
      humidity: 45,
      battery: 85,
    },
    firmwareVersion: 'v1.2.0',
  },
  {
    id: '2',
    name: 'Bedroom',
    isOnline: true,
    lastSeen: new Date(),
    metrics: {
      co2: 520,
      temperature: 20.8,
      humidity: 52,
      battery: 72,
    },
    firmwareVersion: 'v1.2.0',
  },
  {
    id: '3',
    name: 'Office',
    isOnline: false,
    lastSeen: new Date(Date.now() - 3600000),
    metrics: {
      battery: 15,
    },
    firmwareVersion: 'v1.1.0',
  },
];

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
  const onlineDevices = mockDevices.filter((d) => d.isOnline).length;
  const totalDevices = mockDevices.length;
  const { avgCO2, avgTemp, avgHumidity } = calculateAverages(mockDevices);

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
          <p className="text-[hsl(228,10%,50%)] mt-1">
            {onlineDevices} of {totalDevices} devices online
          </p>
        </div>
        <Button
          variant="primary"
          leftIcon={<Plus className="h-4 w-4" />}
        >
          Add Device
        </Button>
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
              'bg-[hsl(187,100%,50%,0.1)]'
            )}
          >
            <Wind className="h-6 w-6 text-[hsl(187,100%,50%)]" />
          </div>
          <div>
            <p className="text-sm text-[hsl(228,10%,50%)]">Avg. CO₂</p>
            {avgCO2 !== null ? (
              <p className="text-2xl font-bold font-mono text-white">
                {avgCO2}
                <span className="text-sm font-normal text-[hsl(228,10%,50%)] ml-1">
                  ppm
                </span>
              </p>
            ) : (
              <p className="text-lg text-[hsl(228,10%,40%)]">No data</p>
            )}
          </div>
        </GlassCard>

        {/* Average Temperature */}
        <GlassCard className="flex items-center gap-4">
          <div
            className={cn(
              'h-12 w-12 rounded-xl flex items-center justify-center',
              'bg-[hsl(38,92%,50%,0.1)]'
            )}
          >
            <Thermometer className="h-6 w-6 text-[hsl(38,92%,50%)]" />
          </div>
          <div>
            <p className="text-sm text-[hsl(228,10%,50%)]">Avg. Temperature</p>
            {avgTemp !== null ? (
              <p className="text-2xl font-bold font-mono text-white">
                {avgTemp.toFixed(1)}
                <span className="text-sm font-normal text-[hsl(228,10%,50%)] ml-1">
                  °C
                </span>
              </p>
            ) : (
              <p className="text-lg text-[hsl(228,10%,40%)]">No data</p>
            )}
          </div>
        </GlassCard>

        {/* Average Humidity */}
        <GlassCard className="flex items-center gap-4">
          <div
            className={cn(
              'h-12 w-12 rounded-xl flex items-center justify-center',
              'bg-[hsl(199,89%,48%,0.1)]'
            )}
          >
            <Droplets className="h-6 w-6 text-[hsl(199,89%,48%)]" />
          </div>
          <div>
            <p className="text-sm text-[hsl(228,10%,50%)]">Avg. Humidity</p>
            {avgHumidity !== null ? (
              <p className="text-2xl font-bold font-mono text-white">
                {avgHumidity}
                <span className="text-sm font-normal text-[hsl(228,10%,50%)] ml-1">
                  %
                </span>
              </p>
            ) : (
              <p className="text-lg text-[hsl(228,10%,40%)]">No data</p>
            )}
          </div>
        </GlassCard>
      </motion.div>

      {/* Devices section */}
      <motion.div variants={fadeInUp}>
        <h2 className="text-lg font-semibold text-white mb-4">Your Devices</h2>
        <DevicesGrid devices={mockDevices} />
      </motion.div>
    </motion.div>
  );
}
