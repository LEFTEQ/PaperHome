import { useParams, Link } from 'react-router-dom';
import {
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ResponsiveContainer,
} from 'recharts';
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Tabs, TabsList, TabsTrigger, TabsContent } from '@/components/ui/tabs';
import {
  ArrowLeft,
  Thermometer,
  Droplets,
  Wind,
  Battery,
  Wifi,
  Lightbulb,
  Flame,
  Settings2,
} from 'lucide-react';

// Mock chart data
const generateChartData = (hours: number, metric: 'co2' | 'temperature' | 'humidity') => {
  const data = [];
  const now = new Date();
  for (let i = hours; i >= 0; i--) {
    const time = new Date(now.getTime() - i * 3600000);
    let value;
    if (metric === 'co2') {
      value = 500 + Math.random() * 300 + Math.sin(i / 4) * 100;
    } else if (metric === 'temperature') {
      value = 20 + Math.random() * 4 + Math.sin(i / 6) * 2;
    } else {
      value = 40 + Math.random() * 20 + Math.sin(i / 5) * 10;
    }
    data.push({
      time: time.toLocaleTimeString('en-US', { hour: '2-digit', minute: '2-digit' }),
      value: Math.round(value * 10) / 10,
    });
  }
  return data;
};

// Mock Hue rooms
const mockHueRooms = [
  { roomId: '1', roomName: 'Living Room', isOn: true, brightness: 80 },
  { roomId: '2', roomName: 'Kitchen', isOn: true, brightness: 100 },
  { roomId: '3', roomName: 'Bedroom', isOn: false, brightness: 0 },
  { roomId: '4', roomName: 'Bathroom', isOn: true, brightness: 60 },
];

// Mock Tado rooms
const mockTadoRooms = [
  { roomId: '1', roomName: 'Living Room', currentTemp: 21.5, targetTemp: 22, isHeating: true, humidity: 45 },
  { roomId: '2', roomName: 'Bedroom', currentTemp: 19.8, targetTemp: 20, isHeating: false, humidity: 52 },
  { roomId: '3', roomName: 'Office', currentTemp: 23.1, targetTemp: 22, isHeating: false, humidity: 38 },
];

// Custom chart tooltip
const CustomTooltip = ({ active, payload, label }: any) => {
  if (active && payload && payload.length) {
    return (
      <div className="bg-[hsl(35,15%,96%)] border border-[hsl(30,15%,85%)] rounded-lg px-3 py-2 shadow-md">
        <p className="text-xs text-[hsl(30,10%,50%)]">{label}</p>
        <p className="text-sm font-semibold font-mono text-[hsl(30,10%,20%)]">
          {payload[0].value}
        </p>
      </div>
    );
  }
  return null;
};

function SensorChart({ title, icon: Icon, data, unit, color }: {
  title: string;
  icon: any;
  data: { time: string; value: number }[];
  unit: string;
  color: string;
}) {
  const latestValue = data[data.length - 1]?.value;

  return (
    <Card>
      <CardHeader className="pb-2">
        <div className="flex items-center justify-between">
          <div className="flex items-center gap-2">
            <Icon className="h-4 w-4 text-[hsl(30,10%,50%)]" />
            <CardTitle className="text-sm font-medium">{title}</CardTitle>
          </div>
          <div className="text-right">
            <span className="text-2xl font-semibold font-mono text-[hsl(30,10%,20%)]">
              {latestValue}
            </span>
            <span className="text-sm text-[hsl(30,10%,50%)] ml-1">{unit}</span>
          </div>
        </div>
      </CardHeader>
      <CardContent>
        <div className="h-48">
          <ResponsiveContainer width="100%" height="100%">
            <LineChart data={data}>
              <CartesianGrid strokeDasharray="3 3" stroke="hsl(30,15%,90%)" />
              <XAxis
                dataKey="time"
                tick={{ fontSize: 10, fill: 'hsl(30,10%,50%)' }}
                axisLine={{ stroke: 'hsl(30,15%,85%)' }}
                tickLine={false}
              />
              <YAxis
                tick={{ fontSize: 10, fill: 'hsl(30,10%,50%)' }}
                axisLine={{ stroke: 'hsl(30,15%,85%)' }}
                tickLine={false}
              />
              <Tooltip content={<CustomTooltip />} />
              <Line
                type="monotone"
                dataKey="value"
                stroke={color}
                strokeWidth={2}
                dot={false}
                activeDot={{ r: 4, fill: color }}
              />
            </LineChart>
          </ResponsiveContainer>
        </div>
      </CardContent>
    </Card>
  );
}

export function DeviceDetailPage() {
  const { id } = useParams();

  // Mock device data
  const device = {
    id,
    deviceId: 'A0B1C2D3E4F5',
    name: 'Living Room',
    isOnline: true,
    battery: 85,
    firmwareVersion: '1.2.0',
  };

  const co2Data = generateChartData(24, 'co2');
  const tempData = generateChartData(24, 'temperature');
  const humidityData = generateChartData(24, 'humidity');

  return (
    <div className="p-8">
      {/* Header */}
      <div className="flex items-center gap-4 mb-8">
        <Link to="/devices">
          <Button variant="ghost" size="sm" className="gap-2">
            <ArrowLeft className="h-4 w-4" />
            Back
          </Button>
        </Link>
        <div className="flex-1">
          <div className="flex items-center gap-3">
            <h1 className="text-2xl font-semibold text-[hsl(30,10%,15%)]">
              {device.name}
            </h1>
            <Badge variant={device.isOnline ? 'success' : 'outline'}>
              <Wifi className="h-3 w-3 mr-1" />
              {device.isOnline ? 'Online' : 'Offline'}
            </Badge>
          </div>
          <p className="text-[hsl(30,10%,45%)] font-mono text-sm mt-1">
            {device.deviceId}
          </p>
        </div>
        <div className="flex items-center gap-4 text-sm">
          <div className="flex items-center gap-2">
            <Battery className="h-4 w-4 text-[hsl(140,30%,40%)]" />
            <span className="font-medium">{device.battery}%</span>
          </div>
          <div className="text-[hsl(30,10%,50%)] font-mono">
            v{device.firmwareVersion}
          </div>
          <Button variant="outline" size="sm">
            <Settings2 className="h-4 w-4" />
          </Button>
        </div>
      </div>

      {/* Tabs */}
      <Tabs defaultValue="sensors">
        <TabsList>
          <TabsTrigger value="sensors">
            <Thermometer className="h-4 w-4 mr-2" />
            Sensors
          </TabsTrigger>
          <TabsTrigger value="hue">
            <Lightbulb className="h-4 w-4 mr-2" />
            Hue Lights
          </TabsTrigger>
          <TabsTrigger value="tado">
            <Flame className="h-4 w-4 mr-2" />
            Tado Heating
          </TabsTrigger>
        </TabsList>

        {/* Sensors Tab */}
        <TabsContent value="sensors">
          <div className="grid grid-cols-1 lg:grid-cols-2 xl:grid-cols-3 gap-4">
            <SensorChart
              title="CO2 Level"
              icon={Wind}
              data={co2Data}
              unit="ppm"
              color="hsl(140,30%,40%)"
            />
            <SensorChart
              title="Temperature"
              icon={Thermometer}
              data={tempData}
              unit="C"
              color="hsl(25,60%,50%)"
            />
            <SensorChart
              title="Humidity"
              icon={Droplets}
              data={humidityData}
              unit="%"
              color="hsl(200,50%,50%)"
            />
          </div>
        </TabsContent>

        {/* Hue Tab */}
        <TabsContent value="hue">
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-4 gap-4">
            {mockHueRooms.map((room) => (
              <Card key={room.roomId} className="overflow-hidden">
                <div
                  className={`h-2 ${
                    room.isOn
                      ? 'bg-[hsl(45,80%,55%)]'
                      : 'bg-[hsl(30,10%,85%)]'
                  }`}
                  style={{
                    opacity: room.isOn ? room.brightness / 100 : 1,
                  }}
                />
                <CardContent className="pt-4">
                  <div className="flex items-center justify-between mb-3">
                    <h3 className="font-medium text-[hsl(30,10%,20%)]">
                      {room.roomName}
                    </h3>
                    <Badge variant={room.isOn ? 'success' : 'outline'}>
                      {room.isOn ? 'On' : 'Off'}
                    </Badge>
                  </div>
                  <div className="flex items-center gap-2">
                    <Lightbulb
                      className={`h-5 w-5 ${
                        room.isOn
                          ? 'text-[hsl(45,80%,50%)]'
                          : 'text-[hsl(30,10%,60%)]'
                      }`}
                    />
                    <div className="flex-1 h-2 bg-[hsl(30,10%,90%)] rounded-full overflow-hidden">
                      <div
                        className="h-full bg-[hsl(45,80%,55%)] transition-all"
                        style={{ width: `${room.brightness}%` }}
                      />
                    </div>
                    <span className="text-sm font-mono text-[hsl(30,10%,50%)]">
                      {room.brightness}%
                    </span>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>
        </TabsContent>

        {/* Tado Tab */}
        <TabsContent value="tado">
          <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4">
            {mockTadoRooms.map((room) => (
              <Card key={room.roomId}>
                <CardContent className="pt-5">
                  <div className="flex items-center justify-between mb-4">
                    <h3 className="font-medium text-[hsl(30,10%,20%)]">
                      {room.roomName}
                    </h3>
                    <Badge
                      variant={room.isHeating ? 'warning' : 'outline'}
                      className="flex items-center gap-1"
                    >
                      <Flame
                        className={`h-3 w-3 ${
                          room.isHeating ? 'text-[hsl(25,70%,50%)]' : ''
                        }`}
                      />
                      {room.isHeating ? 'Heating' : 'Idle'}
                    </Badge>
                  </div>

                  <div className="flex items-center justify-between mb-3">
                    <div>
                      <p className="text-sm text-[hsl(30,10%,50%)]">Current</p>
                      <p className="text-3xl font-semibold font-mono text-[hsl(30,10%,15%)]">
                        {room.currentTemp}
                        <span className="text-base text-[hsl(30,10%,50%)]">C</span>
                      </p>
                    </div>
                    <div className="text-right">
                      <p className="text-sm text-[hsl(30,10%,50%)]">Target</p>
                      <p className="text-xl font-medium font-mono text-[hsl(25,60%,45%)]">
                        {room.targetTemp}C
                      </p>
                    </div>
                  </div>

                  <div className="flex items-center gap-2 text-sm text-[hsl(30,10%,50%)]">
                    <Droplets className="h-4 w-4" />
                    <span>Humidity: {room.humidity}%</span>
                  </div>
                </CardContent>
              </Card>
            ))}
          </div>
        </TabsContent>
      </Tabs>
    </div>
  );
}
