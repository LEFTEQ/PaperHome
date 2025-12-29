import { Link } from 'react-router-dom';
import { motion } from 'framer-motion';
import { Cpu, Wind, Thermometer, Droplets, Battery, ChevronRight } from 'lucide-react';
import { GlassCard } from '@/components/ui/glass-card';
import { StatusIndicator } from '@/components/ui/status-indicator';
import { Badge, CO2Badge } from '@/components/ui/badge';
import { cn } from '@/lib/utils';
import { fadeInUp } from '@/lib/animations';

export interface DeviceMetrics {
  co2?: number;
  temperature?: number;
  humidity?: number;
  battery?: number;
}

export interface DeviceSummaryCardProps {
  /** Device ID */
  id: string;
  /** Device name */
  name: string;
  /** Is device online */
  isOnline: boolean;
  /** Last seen timestamp */
  lastSeen?: Date;
  /** Device metrics */
  metrics?: DeviceMetrics;
  /** Firmware version */
  firmwareVersion?: string;
  /** Additional className */
  className?: string;
}

function formatLastSeen(date: Date): string {
  const now = new Date();
  const diffMs = now.getTime() - date.getTime();
  const diffMins = Math.floor(diffMs / 60000);
  const diffHours = Math.floor(diffMs / 3600000);
  const diffDays = Math.floor(diffMs / 86400000);

  if (diffMins < 1) return 'Just now';
  if (diffMins < 60) return `${diffMins}m ago`;
  if (diffHours < 24) return `${diffHours}h ago`;
  if (diffDays < 7) return `${diffDays}d ago`;
  return date.toLocaleDateString();
}

export function DeviceSummaryCard({
  id,
  name,
  isOnline,
  lastSeen,
  metrics,
  firmwareVersion,
  className,
}: DeviceSummaryCardProps) {
  return (
    <Link to={`/device/${id}`}>
      <GlassCard
        interactive
        className={cn(
          'group h-full cursor-pointer',
          isOnline && 'border-success/20',
          className
        )}
      >
        {/* Header */}
        <div className="flex items-center justify-between mb-4">
          <div className="flex items-center gap-3">
            <div
              className={cn(
                'h-10 w-10 rounded-xl flex items-center justify-center',
                isOnline
                  ? 'bg-success-bg'
                  : 'bg-glass-hover'
              )}
            >
              <Cpu
                className={cn(
                  'h-5 w-5',
                  isOnline
                    ? 'text-online'
                    : 'text-text-subtle'
                )}
              />
            </div>
            <div>
              <div className="flex items-center gap-2">
                <h3 className="text-sm font-semibold text-white">{name}</h3>
                <StatusIndicator
                  status={isOnline ? 'online' : 'offline'}
                  size="sm"
                />
              </div>
              {lastSeen && (
                <p className="text-xs text-text-muted">
                  {isOnline ? 'Active' : `Last seen ${formatLastSeen(lastSeen)}`}
                </p>
              )}
            </div>
          </div>

          <ChevronRight
            className={cn(
              'h-5 w-5 text-text-subtle',
              'group-hover:text-accent',
              'group-hover:translate-x-1',
              'transition-all duration-200'
            )}
          />
        </div>

        {/* Metrics grid */}
        {isOnline && metrics ? (
          <div className="grid grid-cols-2 gap-3">
            {/* CO2 */}
            {metrics.co2 !== undefined && (
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-text-muted">
                  <Wind className="h-3.5 w-3.5" />
                  <span className="text-xs">CO₂</span>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-xl font-mono font-semibold text-white">
                    {metrics.co2}
                  </span>
                  <span className="text-xs text-text-muted">ppm</span>
                </div>
                <CO2Badge value={metrics.co2} size="xs" />
              </div>
            )}

            {/* Temperature */}
            {metrics.temperature !== undefined && (
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-text-muted">
                  <Thermometer className="h-3.5 w-3.5" />
                  <span className="text-xs">Temp</span>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-xl font-mono font-semibold text-white">
                    {metrics.temperature.toFixed(1)}
                  </span>
                  <span className="text-xs text-text-muted">°C</span>
                </div>
              </div>
            )}

            {/* Humidity */}
            {metrics.humidity !== undefined && (
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-text-muted">
                  <Droplets className="h-3.5 w-3.5" />
                  <span className="text-xs">Humidity</span>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-xl font-mono font-semibold text-white">
                    {Math.round(metrics.humidity)}
                  </span>
                  <span className="text-xs text-text-muted">%</span>
                </div>
              </div>
            )}

            {/* Battery */}
            {metrics.battery !== undefined && (
              <div className="space-y-1">
                <div className="flex items-center gap-1.5 text-text-muted">
                  <Battery className="h-3.5 w-3.5" />
                  <span className="text-xs">Battery</span>
                </div>
                <div className="flex items-baseline gap-1">
                  <span className="text-xl font-mono font-semibold text-white">
                    {Math.round(metrics.battery)}
                  </span>
                  <span className="text-xs text-text-muted">%</span>
                </div>
              </div>
            )}
          </div>
        ) : (
          <div className="flex items-center justify-center py-4">
            <p className="text-sm text-text-subtle">
              {isOnline ? 'No data available' : 'Device offline'}
            </p>
          </div>
        )}

        {/* Footer */}
        {firmwareVersion && (
          <div className="mt-4 pt-3 border-t border-glass-border">
            <p className="text-[10px] text-text-subtle font-mono">
              Firmware: {firmwareVersion}
            </p>
          </div>
        )}
      </GlassCard>
    </Link>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Devices Grid - For dashboard
// ─────────────────────────────────────────────────────────────────────────────

export interface Device extends DeviceSummaryCardProps {}

export interface DevicesGridProps {
  devices: Device[];
  className?: string;
}

export function DevicesGrid({ devices, className }: DevicesGridProps) {
  if (devices.length === 0) {
    return (
      <GlassCard className={cn('flex items-center justify-center p-12', className)}>
        <div className="text-center">
          <Cpu className="h-12 w-12 mx-auto text-text-subtle mb-4" />
          <h3 className="text-lg font-semibold text-white mb-2">No devices yet</h3>
          <p className="text-sm text-text-muted max-w-sm">
            Connect your first ESP32 device to start monitoring your home environment.
          </p>
        </div>
      </GlassCard>
    );
  }

  return (
    <motion.div
      className={cn(
        'grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4',
        className
      )}
      initial="initial"
      animate="animate"
      variants={{
        animate: {
          transition: {
            staggerChildren: 0.05,
          },
        },
      }}
    >
      {devices.map((device, index) => (
        <motion.div key={device.id} variants={fadeInUp}>
          <DeviceSummaryCard {...device} />
        </motion.div>
      ))}
    </motion.div>
  );
}

export default DeviceSummaryCard;
