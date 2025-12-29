import { useState } from 'react';
import { Link } from 'react-router-dom';
import { Card, CardContent } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Badge } from '@/components/ui/badge';
import {
  Cpu,
  Plus,
  Search,
  Wifi,
  WifiOff,
  ChevronRight,
  Battery,
} from 'lucide-react';

// Mock data
const mockDevices = [
  {
    id: '1',
    deviceId: 'A0B1C2D3E4F5',
    name: 'Living Room',
    isOnline: true,
    battery: 85,
    lastSeenAt: new Date(),
    firmwareVersion: '1.2.0',
  },
  {
    id: '2',
    deviceId: 'B1C2D3E4F5A0',
    name: 'Bedroom',
    isOnline: true,
    battery: 72,
    lastSeenAt: new Date(),
    firmwareVersion: '1.2.0',
  },
  {
    id: '3',
    deviceId: 'C2D3E4F5A0B1',
    name: 'Office',
    isOnline: false,
    battery: 15,
    lastSeenAt: new Date(Date.now() - 3600000),
    firmwareVersion: '1.1.0',
  },
];

function formatLastSeen(date: Date) {
  const now = new Date();
  const diff = now.getTime() - date.getTime();
  const minutes = Math.floor(diff / 60000);
  const hours = Math.floor(diff / 3600000);
  const days = Math.floor(diff / 86400000);

  if (minutes < 1) return 'Just now';
  if (minutes < 60) return `${minutes}m ago`;
  if (hours < 24) return `${hours}h ago`;
  return `${days}d ago`;
}

export function DevicesPage() {
  const [searchQuery, setSearchQuery] = useState('');
  const [showAddModal, setShowAddModal] = useState(false);

  const filteredDevices = mockDevices.filter(
    (device) =>
      device.name.toLowerCase().includes(searchQuery.toLowerCase()) ||
      device.deviceId.toLowerCase().includes(searchQuery.toLowerCase())
  );

  return (
    <div className="p-8">
      {/* Header */}
      <div className="flex items-center justify-between mb-8">
        <div>
          <h1 className="text-2xl font-semibold text-[hsl(30,10%,15%)]">
            Devices
          </h1>
          <p className="text-[hsl(30,10%,45%)] mt-1">
            Manage your connected devices
          </p>
        </div>
        <Button onClick={() => setShowAddModal(true)}>
          <Plus className="h-4 w-4 mr-2" />
          Add Device
        </Button>
      </div>

      {/* Search */}
      <div className="relative max-w-md mb-6">
        <Search className="absolute left-3 top-1/2 -translate-y-1/2 h-4 w-4 text-[hsl(30,10%,50%)]" />
        <Input
          placeholder="Search devices..."
          value={searchQuery}
          onChange={(e) => setSearchQuery(e.target.value)}
          className="pl-10"
        />
      </div>

      {/* Device list */}
      <div className="space-y-3">
        {filteredDevices.map((device) => (
          <Link key={device.id} to={`/devices/${device.id}`}>
            <Card className="group hover:shadow-md transition-all duration-200 cursor-pointer hover:border-[hsl(25,40%,75%)]">
              <CardContent className="py-4">
                <div className="flex items-center gap-4">
                  {/* Icon */}
                  <div
                    className={`flex h-12 w-12 items-center justify-center rounded-xl ${
                      device.isOnline
                        ? 'bg-[hsl(140,30%,92%)] text-[hsl(140,30%,35%)]'
                        : 'bg-[hsl(30,10%,90%)] text-[hsl(30,10%,50%)]'
                    }`}
                  >
                    <Cpu className="h-6 w-6" />
                  </div>

                  {/* Info */}
                  <div className="flex-1 min-w-0">
                    <div className="flex items-center gap-2">
                      <h3 className="font-medium text-[hsl(30,10%,15%)]">
                        {device.name}
                      </h3>
                      <Badge
                        variant={device.isOnline ? 'success' : 'outline'}
                        className="flex items-center gap-1"
                      >
                        {device.isOnline ? (
                          <Wifi className="h-3 w-3" />
                        ) : (
                          <WifiOff className="h-3 w-3" />
                        )}
                        {device.isOnline ? 'Online' : 'Offline'}
                      </Badge>
                    </div>
                    <p className="text-sm text-[hsl(30,10%,50%)] mt-0.5 font-mono">
                      {device.deviceId}
                    </p>
                  </div>

                  {/* Status */}
                  <div className="hidden sm:flex items-center gap-6 text-sm">
                    <div className="text-right">
                      <p className="text-[hsl(30,10%,50%)]">Last seen</p>
                      <p className="text-[hsl(30,10%,25%)] font-medium">
                        {formatLastSeen(device.lastSeenAt)}
                      </p>
                    </div>
                    <div className="text-right">
                      <p className="text-[hsl(30,10%,50%)]">Battery</p>
                      <p className="text-[hsl(30,10%,25%)] font-medium flex items-center gap-1">
                        <Battery
                          className={`h-4 w-4 ${
                            device.battery < 20
                              ? 'text-[hsl(0,50%,50%)]'
                              : 'text-[hsl(140,30%,40%)]'
                          }`}
                        />
                        {device.battery}%
                      </p>
                    </div>
                    <div className="text-right">
                      <p className="text-[hsl(30,10%,50%)]">Firmware</p>
                      <p className="text-[hsl(30,10%,25%)] font-medium font-mono">
                        v{device.firmwareVersion}
                      </p>
                    </div>
                  </div>

                  <ChevronRight className="h-5 w-5 text-[hsl(30,10%,60%)] opacity-0 group-hover:opacity-100 transition-opacity" />
                </div>
              </CardContent>
            </Card>
          </Link>
        ))}

        {filteredDevices.length === 0 && (
          <div className="text-center py-12">
            <Cpu className="h-12 w-12 mx-auto text-[hsl(30,10%,70%)] mb-4" />
            <h3 className="text-lg font-medium text-[hsl(30,10%,30%)]">
              No devices found
            </h3>
            <p className="text-[hsl(30,10%,50%)] mt-1">
              {searchQuery
                ? 'Try a different search term'
                : 'Add your first device to get started'}
            </p>
          </div>
        )}
      </div>

      {/* Add Device Modal - simplified placeholder */}
      {showAddModal && (
        <div className="fixed inset-0 bg-black/30 flex items-center justify-center z-50">
          <Card className="w-full max-w-md mx-4">
            <CardContent className="pt-6">
              <h2 className="text-lg font-semibold mb-4">Add New Device</h2>
              <Input
                label="Device ID"
                placeholder="Enter MAC address (e.g., A0B1C2D3E4F5)"
                className="mb-4"
              />
              <Input
                label="Device Name"
                placeholder="Give your device a name"
                className="mb-6"
              />
              <div className="flex gap-3 justify-end">
                <Button
                  variant="outline"
                  onClick={() => setShowAddModal(false)}
                >
                  Cancel
                </Button>
                <Button onClick={() => setShowAddModal(false)}>
                  Add Device
                </Button>
              </div>
            </CardContent>
          </Card>
        </div>
      )}
    </div>
  );
}
