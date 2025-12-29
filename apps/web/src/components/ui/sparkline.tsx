import { useMemo } from 'react';
import { motion } from 'framer-motion';
import { cn } from '@/lib/utils';

export interface SparklineProps {
  /** Data points to display */
  data: number[];
  /** Width of the sparkline */
  width?: number;
  /** Height of the sparkline */
  height?: number;
  /** Stroke color */
  color?: string;
  /** Fill color (gradient from color to transparent) */
  fillColor?: string;
  /** Show filled area under the line */
  showFill?: boolean;
  /** Stroke width */
  strokeWidth?: number;
  /** Animate the line drawing */
  animate?: boolean;
  /** Additional className */
  className?: string;
}

export function Sparkline({
  data,
  width = 100,
  height = 32,
  color = 'hsl(187, 100%, 50%)',
  fillColor,
  showFill = true,
  strokeWidth = 2,
  animate = true,
  className,
}: SparklineProps) {
  const { path, fillPath, minY, maxY } = useMemo(() => {
    if (!data.length) {
      return { path: '', fillPath: '', minY: 0, maxY: 0 };
    }

    const padding = strokeWidth;
    const chartWidth = width - padding * 2;
    const chartHeight = height - padding * 2;

    const minY = Math.min(...data);
    const maxY = Math.max(...data);
    const range = maxY - minY || 1;

    const points = data.map((value, index) => {
      const x = padding + (index / (data.length - 1)) * chartWidth;
      const y = padding + chartHeight - ((value - minY) / range) * chartHeight;
      return { x, y };
    });

    // Create smooth curve using quadratic bezier
    let path = `M ${points[0].x} ${points[0].y}`;

    for (let i = 1; i < points.length; i++) {
      const prev = points[i - 1];
      const curr = points[i];
      const midX = (prev.x + curr.x) / 2;
      path += ` Q ${prev.x} ${prev.y} ${midX} ${(prev.y + curr.y) / 2}`;
    }

    // Add last point
    const last = points[points.length - 1];
    path += ` T ${last.x} ${last.y}`;

    // Create fill path (closed shape)
    const fillPath = `${path} L ${last.x} ${height - padding} L ${points[0].x} ${height - padding} Z`;

    return { path, fillPath, minY, maxY };
  }, [data, width, height, strokeWidth]);

  if (!data.length) {
    return (
      <div
        className={cn('flex items-center justify-center', className)}
        style={{ width, height }}
      >
        <span className="text-xs text-text-subtle">No data</span>
      </div>
    );
  }

  const gradientId = `sparkline-gradient-${Math.random().toString(36).substr(2, 9)}`;

  return (
    <svg
      width={width}
      height={height}
      className={cn('overflow-visible', className)}
      viewBox={`0 0 ${width} ${height}`}
    >
      {/* Gradient definition */}
      <defs>
        <linearGradient id={gradientId} x1="0" y1="0" x2="0" y2="1">
          <stop offset="0%" stopColor={fillColor || color} stopOpacity="0.3" />
          <stop offset="100%" stopColor={fillColor || color} stopOpacity="0" />
        </linearGradient>
      </defs>

      {/* Fill area */}
      {showFill && (
        <motion.path
          d={fillPath}
          fill={`url(#${gradientId})`}
          initial={animate ? { opacity: 0 } : undefined}
          animate={animate ? { opacity: 1 } : undefined}
          transition={{ duration: 0.5, delay: 0.5 }}
        />
      )}

      {/* Line */}
      <motion.path
        d={path}
        fill="none"
        stroke={color}
        strokeWidth={strokeWidth}
        strokeLinecap="round"
        strokeLinejoin="round"
        initial={animate ? { pathLength: 0, opacity: 0 } : undefined}
        animate={animate ? { pathLength: 1, opacity: 1 } : undefined}
        transition={{
          pathLength: { duration: 1, ease: 'easeInOut' },
          opacity: { duration: 0.3 },
        }}
      />

      {/* Last point indicator */}
      {data.length > 0 && (
        <motion.circle
          cx={width - strokeWidth}
          cy={
            strokeWidth +
            (height - strokeWidth * 2) -
            ((data[data.length - 1] - minY) / (maxY - minY || 1)) *
              (height - strokeWidth * 2)
          }
          r={3}
          fill={color}
          initial={animate ? { scale: 0, opacity: 0 } : undefined}
          animate={animate ? { scale: 1, opacity: 1 } : undefined}
          transition={{ delay: 1, duration: 0.2 }}
        />
      )}
    </svg>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Preset sparkline variants
// ─────────────────────────────────────────────────────────────────────────────

export function CO2Sparkline({ data, ...props }: Omit<SparklineProps, 'color'>) {
  // Color based on current value
  const currentValue = data[data.length - 1] || 0;
  let color: string;

  if (currentValue < 600) {
    color = 'hsl(160, 84%, 45%)'; // Green - excellent
  } else if (currentValue < 1000) {
    color = 'hsl(187, 100%, 50%)'; // Cyan - good
  } else if (currentValue < 1500) {
    color = 'hsl(38, 92%, 50%)'; // Amber - fair
  } else {
    color = 'hsl(0, 72%, 51%)'; // Red - poor
  }

  return <Sparkline data={data} color={color} {...props} />;
}

export function TemperatureSparkline({
  data,
  ...props
}: Omit<SparklineProps, 'color'>) {
  return (
    <Sparkline data={data} color="hsl(38, 92%, 50%)" {...props} />
  );
}

export function HumiditySparkline({
  data,
  ...props
}: Omit<SparklineProps, 'color'>) {
  return (
    <Sparkline data={data} color="hsl(199, 89%, 48%)" {...props} />
  );
}

export default Sparkline;
