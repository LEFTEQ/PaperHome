import { motion } from 'framer-motion';
import { cn } from '@/lib/utils';

export interface DeviceTab {
  id: string;
  name: string;
  isOnline: boolean;
  isUnclaimed: boolean;
}

export interface DeviceTabBarProps {
  devices: DeviceTab[];
  activeDeviceId: string | null;
  onDeviceSelect: (deviceId: string) => void;
  className?: string;
}

export function DeviceTabBar({
  devices,
  activeDeviceId,
  onDeviceSelect,
  className,
}: DeviceTabBarProps) {
  if (devices.length === 0) {
    return null;
  }

  return (
    <div className={cn('flex items-center justify-center', className)}>
      <motion.div
        className="flex items-center gap-2 p-1.5 rounded-2xl bg-glass border border-glass-border backdrop-blur-xl"
        initial={{ opacity: 0, y: -10 }}
        animate={{ opacity: 1, y: 0 }}
        transition={{ duration: 0.3, ease: 'easeOut' }}
      >
        {devices.map((device, index) => (
          <motion.button
            key={device.id}
            onClick={() => onDeviceSelect(device.id)}
            className={cn(
              'device-tab',
              activeDeviceId === device.id && 'active'
            )}
            initial={{ opacity: 0, scale: 0.9 }}
            animate={{ opacity: 1, scale: 1 }}
            transition={{ delay: index * 0.05, duration: 0.2 }}
            whileHover={{ scale: 1.02 }}
            whileTap={{ scale: 0.98 }}
          >
            {/* Status dot */}
            <span
              className={cn(
                'status-dot',
                device.isOnline ? 'status-online' : 'status-offline'
              )}
            />

            {/* Device name */}
            <span className="truncate max-w-[120px]">{device.name}</span>

            {/* NEW badge for unclaimed devices */}
            {device.isUnclaimed && <span className="new-badge">New</span>}
          </motion.button>
        ))}
      </motion.div>
    </div>
  );
}

export default DeviceTabBar;
