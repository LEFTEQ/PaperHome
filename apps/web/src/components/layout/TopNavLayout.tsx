import { Outlet, useNavigate } from 'react-router-dom';
import { useState, useMemo } from 'react';
import { Home, Cpu, Settings, Lightbulb, Thermometer, Activity } from 'lucide-react';
import { useAuth } from '@/context/auth-context';
import { AnimatedBackground } from './AnimatedBackground';
import { CommandBar, CommandItem } from '@/components/ui/command-bar';
import { NotificationDropdown, Notification } from '@/components/ui/notification-dropdown';
import { ProfileDropdown } from '@/components/ui/profile-dropdown';
import { cn } from '@/lib/utils';

/**
 * TopNavLayout
 *
 * Main layout with top navigation bar containing:
 * - Logo (left)
 * - Command bar / search (center)
 * - Notifications + Profile (right)
 */
export function TopNavLayout() {
  const navigate = useNavigate();
  const { user, logout } = useAuth();

  // Mock notifications - replace with real data from context/API
  const [notifications, setNotifications] = useState<Notification[]>([
    {
      id: '1',
      type: 'device_online',
      title: 'Living Room Sensor Online',
      message: 'Device has reconnected to the network',
      timestamp: new Date(Date.now() - 5 * 60000),
      read: false,
    },
    {
      id: '2',
      type: 'warning',
      title: 'High CO2 Level',
      message: 'CO2 in Office is above 1500 ppm',
      timestamp: new Date(Date.now() - 30 * 60000),
      read: false,
    },
  ]);

  // Mock command items - replace with real device data
  const commandItems: CommandItem[] = useMemo(
    () => [
      {
        id: 'dashboard',
        type: 'navigation',
        label: 'Go to Dashboard',
        description: 'View all devices',
        icon: <Home className="h-4 w-4" />,
        keywords: ['home', 'main'],
        onSelect: () => navigate('/dashboard'),
      },
      {
        id: 'settings',
        type: 'navigation',
        label: 'Settings',
        description: 'Account settings',
        icon: <Settings className="h-4 w-4" />,
        keywords: ['preferences', 'account'],
        onSelect: () => navigate('/settings'),
      },
      // Add dynamic device items here from API
      {
        id: 'device-1',
        type: 'device',
        label: 'Living Room',
        description: 'ESP32 Sensor - Online',
        icon: <Cpu className="h-4 w-4" />,
        keywords: ['sensor', 'esp32', 'living'],
        onSelect: () => navigate('/device/device-1'),
      },
      {
        id: 'action-lights-off',
        type: 'action',
        label: 'Turn off all lights',
        description: 'Turn off all Hue lights',
        icon: <Lightbulb className="h-4 w-4" />,
        keywords: ['hue', 'off', 'lights'],
        onSelect: () => {
          // TODO: Implement light control
          console.log('Turning off all lights');
        },
      },
      {
        id: 'action-heating',
        type: 'action',
        label: 'Set heating to 21°C',
        description: 'Set all Tado zones',
        icon: <Thermometer className="h-4 w-4" />,
        keywords: ['tado', 'temperature', 'heat'],
        onSelect: () => {
          // TODO: Implement heating control
          console.log('Setting heating to 21°C');
        },
      },
    ],
    [navigate]
  );

  const handleMarkAsRead = (id: string) => {
    setNotifications((prev) =>
      prev.map((n) => (n.id === id ? { ...n, read: true } : n))
    );
  };

  const handleMarkAllAsRead = () => {
    setNotifications((prev) => prev.map((n) => ({ ...n, read: true })));
  };

  const handleDismissNotification = (id: string) => {
    setNotifications((prev) => prev.filter((n) => n.id !== id));
  };

  const handleClearAllNotifications = () => {
    setNotifications([]);
  };

  const handleLogout = () => {
    logout();
    navigate('/login');
  };

  const handleSettings = () => {
    navigate('/settings');
  };

  return (
    <div className="min-h-screen relative">
      {/* Animated gradient background */}
      <AnimatedBackground />

      {/* Top Navigation Bar */}
      <header
        className={cn(
          'fixed top-0 left-0 right-0 z-40',
          'h-16 px-4 md:px-6',
          'flex items-center justify-between gap-4',
          'bg-[hsl(228,15%,4%)]/80 backdrop-blur-xl',
          'border-b border-white/[0.06]'
        )}
      >
        {/* Left: Logo */}
        <button
          onClick={() => navigate('/dashboard')}
          className={cn(
            'flex items-center gap-2.5',
            'text-white hover:text-[hsl(187,100%,50%)]',
            'transition-colors duration-200'
          )}
        >
          <div
            className={cn(
              'h-8 w-8 rounded-lg',
              'bg-gradient-to-br from-[hsl(187,100%,50%)] to-[hsl(160,84%,39%)]',
              'flex items-center justify-center',
              'shadow-[0_0_15px_hsla(187,100%,50%,0.3)]'
            )}
          >
            <Activity className="h-4 w-4 text-[hsl(228,15%,4%)]" />
          </div>
          <span className="text-lg font-semibold tracking-tight hidden sm:block">
            PaperHome
          </span>
        </button>

        {/* Center: Command Bar */}
        <div className="flex-1 flex justify-center max-w-xl">
          <CommandBar
            items={commandItems}
            placeholder="Search devices, actions..."
            showShortcut
            defaultExpanded
          />
        </div>

        {/* Right: Notifications + Profile */}
        <div className="flex items-center gap-2">
          <NotificationDropdown
            notifications={notifications}
            onMarkAsRead={handleMarkAsRead}
            onMarkAllAsRead={handleMarkAllAsRead}
            onDismiss={handleDismissNotification}
            onClearAll={handleClearAllNotifications}
          />

          <ProfileDropdown
            user={user || { username: 'user' }}
            onSettings={handleSettings}
            onLogout={handleLogout}
          />
        </div>
      </header>

      {/* Main Content */}
      <main className="pt-16">
        <div className="container mx-auto px-4 md:px-6 py-6">
          <Outlet />
        </div>
      </main>
    </div>
  );
}

export default TopNavLayout;
