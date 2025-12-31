import { LucideIcon } from 'lucide-react';
import { BentoCard } from '@/components/ui/glass-card';
import { Sparkline } from '@/components/ui/sparkline';
import { AnimatedNumber } from '@/components/ui/animated-number';
import { cn } from '@/lib/utils';

export type MetricType = 'co2' | 'temperature' | 'humidity' | 'pressure' | 'battery' | 'iaq';
export type SensorSource = 'STCC4' | 'BME688';
export type BentoSize = '1x1' | '2x1' | '1x2' | '2x2';

export interface SensorStats {
  min: number;
  max: number;
  avg: number;
}

export interface SensorWidgetProps {
  /** Metric type for styling and formatting */
  metricType: MetricType;
  /** Sensor source label */
  sensorSource?: SensorSource;
  /** Current value */
  value: number | null;
  /** Unit of measurement */
  unit: string;
  /** Historical data for sparkline */
  chartData: number[];
  /** Min/Max/Avg statistics */
  stats: SensorStats | null;
  /** Accent color for the widget */
  color: string;
  /** Icon component */
  icon: LucideIcon;
  /** Bento grid size */
  bentoSize: BentoSize;
  /** Additional className for grid positioning */
  className?: string;
  /** Click handler */
  onClick?: () => void;
  /** IAQ accuracy (0-3), only for IAQ widgets */
  iaqAccuracy?: number;
}

/** Format value based on metric type */
function formatValue(value: number, metricType: MetricType): { display: number; decimals: number } {
  switch (metricType) {
    case 'co2':
      return { display: Math.round(value), decimals: 0 };
    case 'temperature':
      return { display: value, decimals: 1 };
    case 'humidity':
      return { display: Math.round(value), decimals: 0 };
    case 'pressure':
      return { display: value, decimals: 1 };
    case 'battery':
      return { display: Math.round(value), decimals: 0 };
    case 'iaq':
      return { display: Math.round(value), decimals: 0 };
    default:
      return { display: value, decimals: 1 };
  }
}

/** Get metric card gradient class */
function getMetricCardClass(metricType: MetricType): string {
  switch (metricType) {
    case 'co2':
      return 'metric-card metric-card-co2';
    case 'temperature':
      return 'metric-card metric-card-temperature';
    case 'humidity':
      return 'metric-card metric-card-humidity';
    case 'pressure':
      return 'metric-card metric-card-pressure';
    case 'battery':
      return 'metric-card metric-card-battery';
    case 'iaq':
      return 'metric-card metric-card-iaq';
    default:
      return 'metric-card';
  }
}

/** Get metric display name */
function getMetricLabel(metricType: MetricType): string {
  switch (metricType) {
    case 'co2':
      return 'CO₂';
    case 'temperature':
      return 'Temperature';
    case 'humidity':
      return 'Humidity';
    case 'pressure':
      return 'Pressure';
    case 'battery':
      return 'Battery';
    case 'iaq':
      return 'Air Quality';
    default:
      return metricType;
  }
}

/** Format stat value for display */
function formatStatValue(value: number, metricType: MetricType, unit: string): string {
  const { decimals } = formatValue(value, metricType);
  const formatted = decimals > 0 ? value.toFixed(decimals) : Math.round(value).toString();

  // Shorten unit for stats display
  const shortUnit = unit === 'ppm' ? '' : unit === 'hPa' ? '' : unit;
  return `${formatted}${shortUnit}`;
}

export function SensorWidget({
  metricType,
  sensorSource,
  value,
  unit,
  chartData,
  stats,
  color,
  icon: Icon,
  bentoSize,
  className,
  onClick,
  iaqAccuracy,
}: SensorWidgetProps) {
  const isLarge = bentoSize === '2x2';
  const isWide = bentoSize === '2x1' || bentoSize === '2x2';
  const metricCardClass = getMetricCardClass(metricType);
  const label = getMetricLabel(metricType);

  // Handle null value - show placeholder
  if (value === null) {
    return (
      <BentoCard
        bentoSize={bentoSize}
        className={cn(metricCardClass, 'opacity-50', className)}
      >
        <div className="flex items-center gap-2 mb-3">
          <div
            className="flex items-center justify-center rounded-xl h-8 w-8"
            style={{ backgroundColor: `${color}15` }}
          >
            <Icon className="h-4 w-4" style={{ color }} />
          </div>
          <span className="text-xs text-text-muted uppercase tracking-wider font-medium">
            {label}
          </span>
          {sensorSource && (
            <span className="text-[10px] text-text-subtle font-mono ml-auto px-1.5 py-0.5 rounded bg-white/5">
              {sensorSource}
            </span>
          )}
        </div>
        <div className="flex-1 flex items-center justify-center">
          <span className="text-text-subtle text-sm">No data</span>
        </div>
      </BentoCard>
    );
  }

  const { display, decimals } = formatValue(value, metricType);

  return (
    <BentoCard
      bentoSize={bentoSize}
      interactive={!!onClick}
      onClick={onClick}
      className={cn(metricCardClass, 'flex flex-col', className)}
    >
      {/* Header */}
      <div className="flex items-center gap-2 mb-3">
        <div
          className={cn(
            'flex items-center justify-center rounded-xl',
            isLarge ? 'h-10 w-10' : 'h-8 w-8'
          )}
          style={{ backgroundColor: `${color}15` }}
        >
          <Icon
            className={isLarge ? 'h-5 w-5' : 'h-4 w-4'}
            style={{ color }}
          />
        </div>
        <span className="text-xs text-text-muted uppercase tracking-wider font-medium">
          {label}
        </span>
        {sensorSource && (
          <span className="text-[10px] text-text-subtle font-mono ml-auto px-1.5 py-0.5 rounded bg-white/5">
            {sensorSource}
          </span>
        )}
        {/* IAQ Accuracy dots */}
        {metricType === 'iaq' && iaqAccuracy !== undefined && (
          <div className="flex items-center gap-1 ml-auto">
            {[0, 1, 2].map((i) => (
              <div
                key={i}
                className={cn(
                  'w-1.5 h-1.5 rounded-full transition-colors',
                  i < iaqAccuracy ? 'bg-teal-400' : 'bg-white/10'
                )}
              />
            ))}
          </div>
        )}
      </div>

      {/* Value Display */}
      <div className={cn(
        'flex items-baseline gap-1',
        isLarge ? 'mb-4' : 'mb-2'
      )}>
        <span
          className={cn(
            'font-mono font-semibold tracking-tight text-white',
            isLarge ? 'text-5xl' : isWide ? 'text-4xl' : 'text-3xl'
          )}
        >
          <AnimatedNumber value={display} decimals={decimals} />
        </span>
        <span className={cn(
          'font-mono text-text-muted',
          isLarge ? 'text-lg' : 'text-sm'
        )}>
          {unit}
        </span>
      </div>

      {/* Sparkline */}
      {chartData.length > 0 && (
        <div className={cn('flex-1 min-h-0', isLarge ? 'mb-4' : 'mb-2')}>
          <Sparkline
            data={chartData}
            color={color}
            height={isLarge ? 80 : isWide ? 48 : 36}
            animate
          />
        </div>
      )}

      {/* Stats Row */}
      {stats && (
        <div className={cn(
          'flex items-center gap-3 pt-2 border-t border-white/5',
          isLarge ? 'text-xs' : 'text-[10px]'
        )}>
          <div className="flex items-center gap-1">
            <span className="text-text-subtle">L:</span>
            <span className="font-mono text-text-secondary">
              {formatStatValue(stats.min, metricType, unit)}
            </span>
          </div>
          <div className="flex items-center gap-1">
            <span className="text-text-subtle">H:</span>
            <span className="font-mono text-text-secondary">
              {formatStatValue(stats.max, metricType, unit)}
            </span>
          </div>
          <div className="flex items-center gap-1">
            <span className="text-text-subtle">Avg:</span>
            <span className="font-mono text-text-secondary">
              {formatStatValue(stats.avg, metricType, unit)}
            </span>
          </div>
        </div>
      )}
    </BentoCard>
  );
}

export default SensorWidget;
