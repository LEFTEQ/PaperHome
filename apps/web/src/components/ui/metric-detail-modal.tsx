import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { X, TrendingUp, TrendingDown, Minus } from 'lucide-react';
import { cn } from '@/lib/utils';
import { Sparkline } from './sparkline';

export type MetricType = 'co2' | 'temperature' | 'humidity' | 'battery';
export type TimeRange = '24h' | '7d' | '30d';

export interface MetricDetailModalProps {
  isOpen: boolean;
  onClose: () => void;
  metricType: MetricType;
  currentValue: number;
  unit: string;
  chartData: number[];
  trend?: 'up' | 'down' | 'stable';
  trendValue?: number;
  min?: number;
  max?: number;
  avg?: number;
}

const metricConfig: Record<
  MetricType,
  { label: string; color: string; gradientClass: string }
> = {
  co2: {
    label: 'CO₂ Level',
    color: '#00e5ff',
    gradientClass: 'metric-card-co2',
  },
  temperature: {
    label: 'Temperature',
    color: '#f59e0b',
    gradientClass: 'metric-card-temperature',
  },
  humidity: {
    label: 'Humidity',
    color: '#8b5cf6',
    gradientClass: 'metric-card-humidity',
  },
  battery: {
    label: 'Battery',
    color: '#22c55e',
    gradientClass: 'metric-card-battery',
  },
};

const timeRanges: { value: TimeRange; label: string }[] = [
  { value: '24h', label: '24 Hours' },
  { value: '7d', label: '7 Days' },
  { value: '30d', label: '30 Days' },
];

export function MetricDetailModal({
  isOpen,
  onClose,
  metricType,
  currentValue,
  unit,
  chartData,
  trend,
  trendValue,
  min,
  max,
  avg,
}: MetricDetailModalProps) {
  const [selectedRange, setSelectedRange] = useState<TimeRange>('24h');
  const config = metricConfig[metricType];

  const TrendIcon =
    trend === 'up' ? TrendingUp : trend === 'down' ? TrendingDown : Minus;

  return (
    <AnimatePresence>
      {isOpen && (
        <>
          {/* Backdrop */}
          <motion.div
            className="fixed inset-0 z-50 bg-black/60 backdrop-blur-sm"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
            onClick={onClose}
          />

          {/* Modal */}
          <motion.div
            className="fixed inset-0 z-50 flex items-center justify-center p-4"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            exit={{ opacity: 0 }}
          >
            <motion.div
              className={cn(
                'relative w-full max-w-2xl rounded-3xl',
                'bg-bg-surface border border-glass-border',
                'shadow-2xl overflow-hidden',
                'metric-card',
                config.gradientClass
              )}
              initial={{ scale: 0.9, opacity: 0, y: 20 }}
              animate={{ scale: 1, opacity: 1, y: 0 }}
              exit={{ scale: 0.9, opacity: 0, y: 20 }}
              transition={{ type: 'spring', damping: 25, stiffness: 300 }}
              onClick={(e) => e.stopPropagation()}
            >
              {/* Header */}
              <div className="flex items-center justify-between p-6 pb-0">
                <div>
                  <p className="text-sm text-text-muted uppercase tracking-wider font-medium">
                    {config.label}
                  </p>
                  <div className="flex items-baseline gap-2 mt-1">
                    <span className="metric-value-large">
                      {metricType === 'temperature'
                        ? currentValue.toFixed(1)
                        : Math.round(currentValue)}
                    </span>
                    <span className="text-2xl text-text-muted font-mono">
                      {unit}
                    </span>
                  </div>
                </div>

                <button
                  onClick={onClose}
                  className="p-2 rounded-xl bg-glass hover:bg-glass-hover border border-glass-border transition-colors"
                >
                  <X className="w-5 h-5 text-text-muted" />
                </button>
              </div>

              {/* Time range selector */}
              <div className="flex gap-2 px-6 pt-6">
                {timeRanges.map((range) => (
                  <button
                    key={range.value}
                    onClick={() => setSelectedRange(range.value)}
                    className={cn(
                      'px-4 py-2 rounded-lg text-sm font-medium transition-all',
                      selectedRange === range.value
                        ? 'bg-accent/20 text-accent border border-accent/30'
                        : 'bg-glass text-text-muted hover:text-text-secondary border border-transparent'
                    )}
                  >
                    {range.label}
                  </button>
                ))}
              </div>

              {/* Chart area */}
              <div className="p-6">
                <div className="h-48 bg-glass/50 rounded-2xl p-4 border border-glass-border">
                  <Sparkline
                    data={chartData}
                    color={config.color}
                    height={160}
                    showArea={true}
                    strokeWidth={2}
                  />
                </div>
              </div>

              {/* Stats row */}
              <div className="grid grid-cols-4 gap-4 px-6 pb-6">
                {/* Current with trend */}
                <div className="bg-glass rounded-xl p-4 border border-glass-border">
                  <p className="text-xs text-text-muted uppercase tracking-wider mb-1">
                    Current
                  </p>
                  <div className="flex items-center gap-2">
                    <span className="text-xl font-mono font-semibold text-white">
                      {metricType === 'temperature'
                        ? currentValue.toFixed(1)
                        : Math.round(currentValue)}
                    </span>
                    {trend && (
                      <TrendIcon
                        className={cn(
                          'w-4 h-4',
                          trend === 'up' && 'text-warning',
                          trend === 'down' && 'text-success',
                          trend === 'stable' && 'text-text-muted'
                        )}
                      />
                    )}
                  </div>
                  {trendValue !== undefined && (
                    <p className="text-xs text-text-subtle mt-1">
                      {trendValue > 0 ? '+' : ''}
                      {trendValue.toFixed(1)} vs 1h ago
                    </p>
                  )}
                </div>

                {/* Min */}
                <div className="bg-glass rounded-xl p-4 border border-glass-border">
                  <p className="text-xs text-text-muted uppercase tracking-wider mb-1">
                    Min
                  </p>
                  <span className="text-xl font-mono font-semibold text-white">
                    {min !== undefined
                      ? metricType === 'temperature'
                        ? min.toFixed(1)
                        : Math.round(min)
                      : '--'}
                  </span>
                </div>

                {/* Max */}
                <div className="bg-glass rounded-xl p-4 border border-glass-border">
                  <p className="text-xs text-text-muted uppercase tracking-wider mb-1">
                    Max
                  </p>
                  <span className="text-xl font-mono font-semibold text-white">
                    {max !== undefined
                      ? metricType === 'temperature'
                        ? max.toFixed(1)
                        : Math.round(max)
                      : '--'}
                  </span>
                </div>

                {/* Average */}
                <div className="bg-glass rounded-xl p-4 border border-glass-border">
                  <p className="text-xs text-text-muted uppercase tracking-wider mb-1">
                    Average
                  </p>
                  <span className="text-xl font-mono font-semibold text-white">
                    {avg !== undefined
                      ? metricType === 'temperature'
                        ? avg.toFixed(1)
                        : Math.round(avg)
                      : '--'}
                  </span>
                </div>
              </div>
            </motion.div>
          </motion.div>
        </>
      )}
    </AnimatePresence>
  );
}

export default MetricDetailModal;
