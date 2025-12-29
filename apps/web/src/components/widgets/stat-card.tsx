import { ReactNode } from 'react';
import { motion } from 'framer-motion';
import { TrendingUp, TrendingDown, Minus, LucideIcon } from 'lucide-react';
import { GlassCard, BentoCard } from '@/components/ui/glass-card';
import { Sparkline } from '@/components/ui/sparkline';
import { Badge } from '@/components/ui/badge';
import { cn } from '@/lib/utils';
import { fadeInUp } from '@/lib/animations';

export interface StatCardProps {
  /** Title/label for the stat */
  title: string;
  /** Current value */
  value: number | string;
  /** Unit of measurement */
  unit?: string;
  /** Icon to display */
  icon?: LucideIcon;
  /** Historical data for sparkline */
  chartData?: number[];
  /** Trend direction */
  trend?: 'up' | 'down' | 'stable';
  /** Trend percentage */
  trendValue?: number;
  /** Status indicator */
  status?: 'normal' | 'warning' | 'critical' | 'success';
  /** Sparkline color */
  chartColor?: string;
  /** Size variant */
  size?: 'sm' | 'md' | 'lg';
  /** Bento grid size */
  bentoSize?: '1x1' | '2x1' | '1x2' | '2x2';
  /** Additional className */
  className?: string;
  /** Click handler */
  onClick?: () => void;
}

const trendIcons = {
  up: TrendingUp,
  down: TrendingDown,
  stable: Minus,
};

const trendColors = {
  up: 'text-[hsl(160,84%,45%)]',
  down: 'text-[hsl(0,72%,51%)]',
  stable: 'text-[hsl(228,10%,50%)]',
};

const statusColors = {
  normal: {
    icon: 'text-[hsl(187,100%,50%)]',
    bg: 'bg-[hsl(187,100%,50%,0.1)]',
    chart: 'hsl(187, 100%, 50%)',
  },
  success: {
    icon: 'text-[hsl(160,84%,45%)]',
    bg: 'bg-[hsl(160,84%,39%,0.1)]',
    chart: 'hsl(160, 84%, 45%)',
  },
  warning: {
    icon: 'text-[hsl(38,92%,50%)]',
    bg: 'bg-[hsl(38,92%,50%,0.1)]',
    chart: 'hsl(38, 92%, 50%)',
  },
  critical: {
    icon: 'text-[hsl(0,72%,51%)]',
    bg: 'bg-[hsl(0,72%,51%,0.1)]',
    chart: 'hsl(0, 72%, 51%)',
  },
};

export function StatCard({
  title,
  value,
  unit,
  icon: Icon,
  chartData,
  trend,
  trendValue,
  status = 'normal',
  chartColor,
  size = 'md',
  bentoSize,
  className,
  onClick,
}: StatCardProps) {
  const colors = statusColors[status];
  const TrendIcon = trend ? trendIcons[trend] : null;

  const sizeClasses = {
    sm: {
      value: 'text-2xl',
      icon: 'h-8 w-8',
      iconInner: 'h-4 w-4',
      chart: { width: 80, height: 24 },
    },
    md: {
      value: 'text-4xl',
      icon: 'h-10 w-10',
      iconInner: 'h-5 w-5',
      chart: { width: 100, height: 32 },
    },
    lg: {
      value: 'text-5xl',
      icon: 'h-12 w-12',
      iconInner: 'h-6 w-6',
      chart: { width: 140, height: 48 },
    },
  };

  const sizes = sizeClasses[size];
  const CardComponent = bentoSize ? BentoCard : GlassCard;

  return (
    <CardComponent
      bentoSize={bentoSize}
      interactive={!!onClick}
      onClick={onClick}
      className={cn('flex flex-col', className)}
    >
      {/* Header */}
      <div className="flex items-center justify-between mb-4">
        <span className="text-sm font-medium text-[hsl(228,10%,60%)]">
          {title}
        </span>
        {Icon && (
          <div
            className={cn(
              'flex items-center justify-center rounded-xl',
              sizes.icon,
              colors.bg
            )}
          >
            <Icon className={cn(sizes.iconInner, colors.icon)} />
          </div>
        )}
      </div>

      {/* Value + Chart */}
      <div className="flex-1 flex items-end justify-between gap-4">
        <div className="flex flex-col">
          {/* Value */}
          <div className="flex items-baseline gap-1">
            <span
              className={cn(
                'font-mono font-semibold tracking-tight text-white',
                sizes.value
              )}
            >
              {typeof value === 'number' ? value.toLocaleString() : value}
            </span>
            {unit && (
              <span className="text-sm font-mono text-[hsl(228,10%,50%)]">
                {unit}
              </span>
            )}
          </div>

          {/* Trend */}
          {trend && trendValue !== undefined && (
            <div className="flex items-center gap-1 mt-1">
              {TrendIcon && (
                <TrendIcon className={cn('h-3.5 w-3.5', trendColors[trend])} />
              )}
              <span
                className={cn('text-xs font-medium', trendColors[trend])}
              >
                {trend === 'up' ? '+' : trend === 'down' ? '-' : ''}
                {Math.abs(trendValue)}%
              </span>
              <span className="text-xs text-[hsl(228,10%,40%)]">vs 1h ago</span>
            </div>
          )}
        </div>

        {/* Sparkline */}
        {chartData && chartData.length > 0 && (
          <Sparkline
            data={chartData}
            color={chartColor || colors.chart}
            width={sizes.chart.width}
            height={sizes.chart.height}
            animate
          />
        )}
      </div>
    </CardComponent>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Preset stat cards for specific metrics
// ─────────────────────────────────────────────────────────────────────────────

export interface SensorStatProps {
  value: number;
  unit?: string;
  chartData?: number[];
  trend?: 'up' | 'down' | 'stable';
  trendValue?: number;
  bentoSize?: '1x1' | '2x1' | '1x2' | '2x2';
  className?: string;
  onClick?: () => void;
}

import { Wind, Thermometer, Droplets, Battery } from 'lucide-react';

export function CO2StatCard({
  value,
  chartData,
  bentoSize = '1x1',
  ...props
}: SensorStatProps) {
  // Determine status based on CO2 level
  let status: 'success' | 'normal' | 'warning' | 'critical';
  if (value < 600) status = 'success';
  else if (value < 1000) status = 'normal';
  else if (value < 1500) status = 'warning';
  else status = 'critical';

  return (
    <StatCard
      title="CO₂"
      value={value}
      unit="ppm"
      icon={Wind}
      chartData={chartData}
      status={status}
      bentoSize={bentoSize}
      {...props}
    />
  );
}

export function TemperatureStatCard({
  value,
  chartData,
  bentoSize = '1x1',
  ...props
}: SensorStatProps) {
  return (
    <StatCard
      title="Temperature"
      value={value.toFixed(1)}
      unit="°C"
      icon={Thermometer}
      chartData={chartData}
      status="normal"
      chartColor="hsl(38, 92%, 50%)"
      bentoSize={bentoSize}
      {...props}
    />
  );
}

export function HumidityStatCard({
  value,
  chartData,
  bentoSize = '1x1',
  ...props
}: SensorStatProps) {
  // Determine status based on humidity level
  let status: 'success' | 'normal' | 'warning';
  if (value >= 40 && value <= 60) status = 'success';
  else if (value >= 30 && value <= 70) status = 'normal';
  else status = 'warning';

  return (
    <StatCard
      title="Humidity"
      value={Math.round(value)}
      unit="%"
      icon={Droplets}
      chartData={chartData}
      status={status}
      chartColor="hsl(199, 89%, 48%)"
      bentoSize={bentoSize}
      {...props}
    />
  );
}

export function BatteryStatCard({
  value,
  bentoSize = '1x1',
  ...props
}: Omit<SensorStatProps, 'chartData'>) {
  // Determine status based on battery level
  let status: 'success' | 'normal' | 'warning' | 'critical';
  if (value > 80) status = 'success';
  else if (value > 50) status = 'normal';
  else if (value > 20) status = 'warning';
  else status = 'critical';

  return (
    <StatCard
      title="Battery"
      value={Math.round(value)}
      unit="%"
      icon={Battery}
      status={status}
      bentoSize={bentoSize}
      size="sm"
      {...props}
    />
  );
}

export default StatCard;
