import { Link } from 'react-router-dom';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import {
  Thermometer,
  Droplets,
  Wind,
  Battery,
  Wifi,
  WifiOff,
  ChevronRight,
  Cpu,
} from 'lucide-react';

// Mock data - will be replaced with API calls
const mockDevices = [
  {
    id: '1',
    deviceId: 'A0B1C2D3E4F5',
    name: 'Living Room',
    isOnline: true,
    co2: 650,
    temperature: 22.5,
    humidity: 45,
    battery: 85,
    lastSeen: new Date(),
  },
  {
    id: '2',
    deviceId: 'B1C2D3E4F5A0',
    name: 'Bedroom',
    isOnline: true,
    co2: 520,
    temperature: 20.8,
    humidity: 52,
    battery: 72,
    lastSeen: new Date(),
  },
  {
    id: '3',
    deviceId: 'C2D3E4F5A0B1',
    name: 'Office',
    isOnline: false,
    co2: null,
    temperature: null,
    humidity: null,
    battery: 15,
    lastSeen: new Date(Date.now() - 3600000),
  },
];

function getCO2Status(co2: number | null) {
  if (co2 === null) return { label: 'N/A', variant: 'outline' as const };
  if (co2 < 600) return { label: 'Excellent', variant: 'success' as const };
  if (co2 < 1000) return { label: 'Good', variant: 'default' as const };
  if (co2 < 1500) return { label: 'Fair', variant: 'warning' as const };
  return { label: 'Poor', variant: 'error' as const };
}

function DeviceCard({ device }: { device: (typeof mockDevices)[0] }) {
  const co2Status = getCO2Status(device.co2);

  return (
    <Link to={`/devices/${device.id}`}>
      <Card className="group hover:shadow-md transition-all duration-200 cursor-pointer hover:border-[hsl(25,40%,75%)]">
        <CardHeader className="pb-3">
          <div className="flex items-start justify-between">
            <div className="flex items-center gap-3">
              <div
                className={`flex h-10 w-10 items-center justify-center rounded-lg ${
                  device.isOnline
                    ? 'bg-[hsl(140,30%,92%)] text-[hsl(140,30%,35%)]'
                    : 'bg-[hsl(30,10%,90%)] text-[hsl(30,10%,50%)]'
                }`}
              >
                <Cpu className="h-5 w-5" />
              </div>
              <div>
                <CardTitle className="text-base">{device.name}</CardTitle>
                <div className="flex items-center gap-2 mt-0.5">
                  {device.isOnline ? (
                    <Wifi className="h-3 w-3 text-[hsl(140,30%,40%)]" />
                  ) : (
                    <WifiOff className="h-3 w-3 text-[hsl(30,10%,55%)]" />
                  )}
                  <span className="text-xs text-[hsl(30,10%,50%)]">
                    {device.isOnline ? 'Online' : 'Offline'}
                  </span>
                </div>
              </div>
            </div>
            <ChevronRight className="h-5 w-5 text-[hsl(30,10%,60%)] opacity-0 group-hover:opacity-100 transition-opacity" />
          </div>
        </CardHeader>

        <CardContent>
          {device.isOnline ? (
            <div className="grid grid-cols-2 gap-3">
              {/* CO2 */}
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-[hsl(30,10%,50%)]">
                  <Wind className="h-3.5 w-3.5" />
                  <span className="text-xs">CO2</span>
                </div>
                <div className="flex items-baseline gap-2">
                  <span className="text-xl font-semibold font-mono text-[hsl(30,10%,20%)]">
                    {device.co2}
                  </span>
                  <span className="text-xs text-[hsl(30,10%,50%)]">ppm</span>
                </div>
                <Badge variant={co2Status.variant} className="mt-1">
                  {co2Status.label}
                </Badge>
              </div>

              {/* Temperature */}
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-[hsl(30,10%,50%)]">
                  <Thermometer className="h-3.5 w-3.5" />
                  <span className="text-xs">Temp</span>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-xl font-semibold font-mono text-[hsl(30,10%,20%)]">
                    {device.temperature?.toFixed(1)}
                  </span>
                  <span className="text-xs text-[hsl(30,10%,50%)]">C</span>
                </div>
              </div>

              {/* Humidity */}
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-[hsl(30,10%,50%)]">
                  <Droplets className="h-3.5 w-3.5" />
                  <span className="text-xs">Humidity</span>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-xl font-semibold font-mono text-[hsl(30,10%,20%)]">
                    {device.humidity}
                  </span>
                  <span className="text-xs text-[hsl(30,10%,50%)]">%</span>
                </div>
              </div>

              {/* Battery */}
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-[hsl(30,10%,50%)]">
                  <Battery className="h-3.5 w-3.5" />
                  <span className="text-xs">Battery</span>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-xl font-semibold font-mono text-[hsl(30,10%,20%)]">
                    {device.battery}
                  </span>
                  <span className="text-xs text-[hsl(30,10%,50%)]">%</span>
                </div>
              </div>
            </div>
          ) : (
            <div className="flex items-center justify-center h-24 text-[hsl(30,10%,50%)] text-sm">
              Device is offline
            </div>
          )}
        </CardContent>
      </Card>
    </Link>
  );
}

export function DashboardPage() {
  const onlineDevices = mockDevices.filter((d) => d.isOnline).length;
  const totalDevices = mockDevices.length;

  return (
    <div className="p-8">
      {/* Header */}
      <div className="mb-8">
        <h1 className="text-2xl font-semibold text-[hsl(30,10%,15%)]">
          Dashboard
        </h1>
        <p className="text-[hsl(30,10%,45%)] mt-1">
          {onlineDevices} of {totalDevices} devices online
        </p>
      </div>

      {/* Summary cards */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-8">
        <Card>
          <CardContent className="pt-5">
            <div className="flex items-center gap-4">
              <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-[hsl(140,30%,92%)]">
                <Wind className="h-6 w-6 text-[hsl(140,30%,35%)]" />
              </div>
              <div>
                <p className="text-sm text-[hsl(30,10%,50%)]">Avg. CO2</p>
                <p className="text-2xl font-semibold font-mono text-[hsl(30,10%,15%)]">
                  585
                  <span className="text-sm font-normal text-[hsl(30,10%,50%)] ml-1">
                    ppm
                  </span>
                </p>
              </div>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardContent className="pt-5">
            <div className="flex items-center gap-4">
              <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-[hsl(25,50%,92%)]">
                <Thermometer className="h-6 w-6 text-[hsl(25,60%,45%)]" />
              </div>
              <div>
                <p className="text-sm text-[hsl(30,10%,50%)]">Avg. Temp</p>
                <p className="text-2xl font-semibold font-mono text-[hsl(30,10%,15%)]">
                  21.7
                  <span className="text-sm font-normal text-[hsl(30,10%,50%)] ml-1">
                    C
                  </span>
                </p>
              </div>
            </div>
          </CardContent>
        </Card>

        <Card>
          <CardContent className="pt-5">
            <div className="flex items-center gap-4">
              <div className="flex h-12 w-12 items-center justify-center rounded-xl bg-[hsl(200,40%,92%)]">
                <Droplets className="h-6 w-6 text-[hsl(200,40%,45%)]" />
              </div>
              <div>
                <p className="text-sm text-[hsl(30,10%,50%)]">Avg. Humidity</p>
                <p className="text-2xl font-semibold font-mono text-[hsl(30,10%,15%)]">
                  48
                  <span className="text-sm font-normal text-[hsl(30,10%,50%)] ml-1">
                    %
                  </span>
                </p>
              </div>
            </div>
          </CardContent>
        </Card>
      </div>

      {/* Device grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {mockDevices.map((device) => (
          <DeviceCard key={device.id} device={device} />
        ))}
      </div>
    </div>
  );
}
