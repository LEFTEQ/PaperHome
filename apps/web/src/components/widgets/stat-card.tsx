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
  up: 'text-success',
  down: 'text-error',
  stable: 'text-text-muted',
};

const statusColors = {
  normal: {
    icon: 'text-accent',
    bg: 'bg-accent-subtle',
    chart: 'hsl(187, 100%, 50%)',
  },
  success: {
    icon: 'text-success',
    bg: 'bg-success-bg',
    chart: 'hsl(160, 84%, 45%)',
  },
  warning: {
    icon: 'text-warning',
    bg: 'bg-warning-bg',
    chart: 'hsl(38, 92%, 50%)',
  },
  critical: {
    icon: 'text-error',
    bg: 'bg-error-bg',
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
        <span className="text-sm font-medium text-text-muted">
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
              <span className="text-sm font-mono text-text-muted">
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
              <span className="text-xs text-text-subtle">vs 1h ago</span>
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

import { Wind, Thermometer, Droplets, Battery, Gauge, Sun, Cloud, CloudRain, TrendingUp as TrendUp, TrendingDown as TrendDown } from 'lucide-react';

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

// ─────────────────────────────────────────────────────────────────────────────
// BME688 Sensor Cards
// ─────────────────────────────────────────────────────────────────────────────

export interface IAQStatProps extends SensorStatProps {
  accuracy?: number;
}

/**
 * Get IAQ status based on BSEC standard levels
 */
function getIAQStatus(iaq: number): { label: string; status: 'success' | 'normal' | 'warning' | 'critical' } {
  if (iaq <= 50) return { label: 'Excellent', status: 'success' };
  if (iaq <= 100) return { label: 'Good', status: 'normal' };
  if (iaq <= 150) return { label: 'Lightly Polluted', status: 'warning' };
  if (iaq <= 200) return { label: 'Moderately Polluted', status: 'warning' };
  if (iaq <= 250) return { label: 'Heavily Polluted', status: 'critical' };
  if (iaq <= 350) return { label: 'Severely Polluted', status: 'critical' };
  return { label: 'Extremely Polluted', status: 'critical' };
}

export function IAQStatCard({
  value,
  accuracy = 0,
  chartData,
  bentoSize = '2x2',
  className,
  onClick,
}: IAQStatProps) {
  const { label, status } = getIAQStatus(value);
  const colors = statusColors[status];

  return (
    <BentoCard
      bentoSize={bentoSize}
      interactive={!!onClick}
      onClick={onClick}
      className={cn('flex flex-col', className)}
    >
      {/* Header */}
      <div className="flex items-center justify-between mb-2">
        <span className="text-sm font-medium text-text-muted">Air Quality</span>
        {/* Accuracy indicator dots */}
        <div className="flex items-center gap-1">
          {[0, 1, 2].map((i) => (
            <div
              key={i}
              className={cn(
                'w-2 h-2 rounded-full transition-colors',
                i < accuracy ? 'bg-accent' : 'bg-surface-hover'
              )}
            />
          ))}
        </div>
      </div>

      {/* Large value display */}
      <div className="flex-1 flex flex-col items-center justify-center py-4">
        <span className="text-6xl font-mono font-bold text-white">{value}</span>
        <span className={cn(
          'text-lg font-semibold mt-2',
          status === 'success' && 'text-success',
          status === 'normal' && 'text-accent',
          status === 'warning' && 'text-warning',
          status === 'critical' && 'text-error',
        )}>
          {label}
        </span>
      </div>

      {/* Sparkline at bottom */}
      {chartData && chartData.length > 0 && (
        <div className="mt-auto">
          <Sparkline
            data={chartData}
            color={colors.chart}
            width={200}
            height={48}
            animate
          />
        </div>
      )}
    </BentoCard>
  );
}

export interface PressureStatProps extends SensorStatProps {
  pressureTrend?: 'rising' | 'stable' | 'falling';
}

/**
 * Get pressure trend from chart data
 */
function getPressureTrend(chartData?: number[]): 'rising' | 'stable' | 'falling' {
  if (!chartData || chartData.length < 6) return 'stable';
  const recent = chartData.slice(-6);
  const oldest = recent[0];
  const newest = recent[recent.length - 1];
  const diff = newest - oldest;
  if (diff > 2) return 'rising';
  if (diff < -2) return 'falling';
  return 'stable';
}

/**
 * Get weather icon based on pressure and trend
 */
function getWeatherIcon(pressure: number, trend: 'rising' | 'stable' | 'falling') {
  if (pressure > 1020 && trend !== 'falling') {
    return <Sun className="h-6 w-6 text-warning" />;
  }
  if (pressure < 1000 || trend === 'falling') {
    return <CloudRain className="h-6 w-6 text-accent" />;
  }
  return <Cloud className="h-6 w-6 text-text-muted" />;
}

export function PressureStatCard({
  value,
  chartData,
  pressureTrend,
  bentoSize = '2x1',
  className,
  onClick,
}: PressureStatProps) {
  const trend = pressureTrend || getPressureTrend(chartData);

  return (
    <BentoCard
      bentoSize={bentoSize}
      interactive={!!onClick}
      onClick={onClick}
      className={cn('flex flex-col', className)}
    >
      {/* Header */}
      <div className="flex items-center justify-between mb-3">
        <span className="text-sm font-medium text-text-muted">Pressure</span>
        <div
          className={cn(
            'flex items-center justify-center rounded-xl h-10 w-10',
            'bg-purple-500/10'
          )}
        >
          <Gauge className="h-5 w-5 text-purple-400" />
        </div>
      </div>

      {/* Value + Weather */}
      <div className="flex items-end justify-between gap-4">
        <div className="flex flex-col">
          <div className="flex items-baseline gap-1">
            <span className="text-4xl font-mono font-semibold tracking-tight text-white">
              {value.toFixed(1)}
            </span>
            <span className="text-sm font-mono text-text-muted">hPa</span>
          </div>

          {/* Trend indicator */}
          <div className="flex items-center gap-2 mt-1">
            {trend === 'rising' && (
              <>
                <TrendUp className="h-4 w-4 text-success" />
                <span className="text-xs text-success">Rising</span>
              </>
            )}
            {trend === 'falling' && (
              <>
                <TrendDown className="h-4 w-4 text-warning" />
                <span className="text-xs text-warning">Falling</span>
              </>
            )}
            {trend === 'stable' && (
              <>
                <Minus className="h-4 w-4 text-text-muted" />
                <span className="text-xs text-text-muted">Stable</span>
              </>
            )}
          </div>
        </div>

        {/* Weather icon */}
        <div className="flex flex-col items-center gap-1">
          {getWeatherIcon(value, trend)}
          <span className="text-xs text-text-subtle">
            {value > 1020 && trend !== 'falling' ? 'Fair' :
             value < 1000 || trend === 'falling' ? 'Rain likely' : 'Partly cloudy'}
          </span>
        </div>

        {/* Sparkline */}
        {chartData && chartData.length > 0 && (
          <Sparkline
            data={chartData}
            color="hsl(280, 70%, 50%)"
            width={80}
            height={32}
            animate
          />
        )}
      </div>
    </BentoCard>
  );
}

export interface DualSensorProps {
  stcc4Temp: number;
  stcc4Humidity: number;
  bme688Temp: number;
  bme688Humidity: number;
  bentoSize?: '1x1' | '2x1' | '1x2' | '2x2';
  className?: string;
  onClick?: () => void;
}

export function DualSensorCompareCard({
  stcc4Temp,
  stcc4Humidity,
  bme688Temp,
  bme688Humidity,
  bentoSize = '1x2',
  className,
  onClick,
}: DualSensorProps) {
  return (
    <BentoCard
      bentoSize={bentoSize}
      interactive={!!onClick}
      onClick={onClick}
      className={cn('flex flex-col', className)}
    >
      <span className="text-sm font-medium text-text-muted mb-4">Sensor Comparison</span>

      <div className="flex-1 grid grid-cols-2 gap-4">
        {/* STCC4 Column */}
        <div className="flex flex-col items-center justify-center border-r border-border pr-4">
          <span className="text-xs text-text-subtle mb-2 font-medium">STCC4</span>
          <div className="flex items-baseline gap-1">
            <Thermometer className="h-4 w-4 text-warning mr-1" />
            <span className="text-xl font-mono font-semibold text-white">
              {stcc4Temp.toFixed(1)}
            </span>
            <span className="text-xs text-text-muted">°C</span>
          </div>
          <div className="flex items-baseline gap-1 mt-2">
            <Droplets className="h-4 w-4 text-accent mr-1" />
            <span className="text-lg font-mono text-text-secondary">
              {Math.round(stcc4Humidity)}
            </span>
            <span className="text-xs text-text-muted">%</span>
          </div>
        </div>

        {/* BME688 Column */}
        <div className="flex flex-col items-center justify-center pl-4">
          <span className="text-xs text-text-subtle mb-2 font-medium">BME688</span>
          <div className="flex items-baseline gap-1">
            <Thermometer className="h-4 w-4 text-warning mr-1" />
            <span className="text-xl font-mono font-semibold text-white">
              {bme688Temp.toFixed(1)}
            </span>
            <span className="text-xs text-text-muted">°C</span>
          </div>
          <div className="flex items-baseline gap-1 mt-2">
            <Droplets className="h-4 w-4 text-accent mr-1" />
            <span className="text-lg font-mono text-text-secondary">
              {Math.round(bme688Humidity)}
            </span>
            <span className="text-xs text-text-muted">%</span>
          </div>
        </div>
      </div>

      {/* Difference indicator */}
      <div className="mt-4 pt-3 border-t border-border">
        <div className="flex justify-center gap-4 text-xs text-text-subtle">
          <span>Δ Temp: {Math.abs(stcc4Temp - bme688Temp).toFixed(1)}°C</span>
          <span>Δ Humidity: {Math.abs(stcc4Humidity - bme688Humidity).toFixed(0)}%</span>
        </div>
      </div>
    </BentoCard>
  );
}

export default StatCard;
