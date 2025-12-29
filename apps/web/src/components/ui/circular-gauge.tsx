import { useMemo, useCallback } from 'react';
import { motion } from 'framer-motion';
import { Flame } from 'lucide-react';
import { cn } from '@/lib/utils';

export interface CircularGaugeProps {
  /** Current temperature */
  current: number;
  /** Target temperature */
  target: number;
  /** Minimum temperature */
  min?: number;
  /** Maximum temperature */
  max?: number;
  /** Temperature unit */
  unit?: 'C' | 'F';
  /** Is heating active */
  isHeating?: boolean;
  /** Size of the gauge */
  size?: number;
  /** Stroke width of the arc */
  strokeWidth?: number;
  /** Callback when target is changed (for interactive mode) */
  onTargetChange?: (value: number) => void;
  /** Show labels */
  showLabels?: boolean;
  /** Additional className */
  className?: string;
}

export function CircularGauge({
  current,
  target,
  min = 5,
  max = 30,
  unit = 'C',
  isHeating = false,
  size = 200,
  strokeWidth = 12,
  onTargetChange,
  showLabels = true,
  className,
}: CircularGaugeProps) {
  const radius = (size - strokeWidth) / 2;
  const center = size / 2;

  // Arc configuration
  const startAngle = 135; // Start from bottom-left
  const endAngle = 405; // End at bottom-right (270 degrees arc)
  const totalAngle = endAngle - startAngle;

  // Convert value to angle
  const valueToAngle = useCallback(
    (value: number) => {
      const percent = (value - min) / (max - min);
      return startAngle + percent * totalAngle;
    },
    [min, max, startAngle, totalAngle]
  );

  // Convert angle to value
  const angleToValue = useCallback(
    (angle: number) => {
      const percent = (angle - startAngle) / totalAngle;
      return min + percent * (max - min);
    },
    [min, max, startAngle, totalAngle]
  );

  // Generate arc path
  const polarToCartesian = useCallback(
    (angle: number) => {
      const radians = ((angle - 90) * Math.PI) / 180;
      return {
        x: center + radius * Math.cos(radians),
        y: center + radius * Math.sin(radians),
      };
    },
    [center, radius]
  );

  const describeArc = useCallback(
    (startAngleDeg: number, endAngleDeg: number) => {
      const start = polarToCartesian(endAngleDeg);
      const end = polarToCartesian(startAngleDeg);
      const largeArcFlag = endAngleDeg - startAngleDeg <= 180 ? 0 : 1;

      return [
        'M',
        start.x,
        start.y,
        'A',
        radius,
        radius,
        0,
        largeArcFlag,
        0,
        end.x,
        end.y,
      ].join(' ');
    },
    [polarToCartesian, radius]
  );

  // Paths
  const backgroundPath = useMemo(
    () => describeArc(startAngle, endAngle),
    [describeArc, startAngle, endAngle]
  );

  const currentAngle = valueToAngle(Math.max(min, Math.min(max, current)));
  const targetAngle = valueToAngle(Math.max(min, Math.min(max, target)));

  // Progress paths
  const currentPath = useMemo(
    () => describeArc(startAngle, currentAngle),
    [describeArc, startAngle, currentAngle]
  );

  // Target indicator position
  const targetPos = polarToCartesian(targetAngle);

  // Colors
  const currentColor = isHeating
    ? 'hsl(25, 95%, 53%)' // Orange when heating
    : current >= target
    ? 'hsl(160, 84%, 45%)' // Green when at target
    : 'hsl(187, 100%, 50%)'; // Cyan when below target

  const glowColor = isHeating
    ? 'hsla(25, 95%, 53%, 0.4)'
    : 'hsla(187, 100%, 50%, 0.3)';

  // Generate tick marks
  const ticks = useMemo(() => {
    const tickMarks = [];
    const step = 5;
    for (let temp = min; temp <= max; temp += step) {
      const angle = valueToAngle(temp);
      const pos = polarToCartesian(angle);
      const innerPos = polarToCartesian(angle);
      const outerPos = {
        x: center + (radius - 15) * Math.cos(((angle - 90) * Math.PI) / 180),
        y: center + (radius - 15) * Math.sin(((angle - 90) * Math.PI) / 180),
      };
      tickMarks.push({
        temp,
        x1: pos.x,
        y1: pos.y,
        x2: outerPos.x,
        y2: outerPos.y,
        labelX: center + (radius - 30) * Math.cos(((angle - 90) * Math.PI) / 180),
        labelY: center + (radius - 30) * Math.sin(((angle - 90) * Math.PI) / 180),
      });
    }
    return tickMarks;
  }, [min, max, center, radius, valueToAngle, polarToCartesian]);

  return (
    <div className={cn('relative inline-flex flex-col items-center', className)}>
      <svg
        width={size}
        height={size}
        viewBox={`0 0 ${size} ${size}`}
        className="overflow-visible"
      >
        {/* Background arc */}
        <path
          d={backgroundPath}
          fill="none"
          stroke="rgba(255, 255, 255, 0.06)"
          strokeWidth={strokeWidth}
          strokeLinecap="round"
        />

        {/* Current temperature arc */}
        <motion.path
          d={currentPath}
          fill="none"
          stroke={currentColor}
          strokeWidth={strokeWidth}
          strokeLinecap="round"
          initial={{ pathLength: 0 }}
          animate={{ pathLength: 1 }}
          transition={{ duration: 1, ease: 'easeOut' }}
          style={{
            filter: `drop-shadow(0 0 10px ${glowColor})`,
          }}
        />

        {/* Tick marks */}
        {showLabels &&
          ticks.map(({ temp, x1, y1, x2, y2, labelX, labelY }) => (
            <g key={temp}>
              <line
                x1={x1}
                y1={y1}
                x2={x2}
                y2={y2}
                stroke="rgba(255, 255, 255, 0.1)"
                strokeWidth={1}
              />
              <text
                x={labelX}
                y={labelY}
                fill="rgba(255, 255, 255, 0.3)"
                fontSize={10}
                textAnchor="middle"
                dominantBaseline="middle"
                className="font-mono"
              >
                {temp}
              </text>
            </g>
          ))}

        {/* Target indicator */}
        <motion.circle
          cx={targetPos.x}
          cy={targetPos.y}
          r={8}
          fill="white"
          stroke={currentColor}
          strokeWidth={3}
          initial={{ scale: 0 }}
          animate={{ scale: 1 }}
          transition={{ delay: 0.5, type: 'spring', stiffness: 400, damping: 20 }}
          style={{
            filter: `drop-shadow(0 0 8px ${glowColor})`,
          }}
        />
      </svg>

      {/* Center content */}
      <div className="absolute inset-0 flex flex-col items-center justify-center">
        {/* Heating indicator */}
        {isHeating && (
          <motion.div
            className="flex items-center gap-1 mb-1"
            initial={{ opacity: 0, y: 5 }}
            animate={{ opacity: 1, y: 0 }}
          >
            <Flame className="h-4 w-4 text-[hsl(25,95%,53%)]" />
            <span className="text-xs font-medium text-[hsl(25,95%,53%)]">
              Heating
            </span>
          </motion.div>
        )}

        {/* Current temperature */}
        <motion.div
          className="flex items-baseline"
          initial={{ opacity: 0, scale: 0.9 }}
          animate={{ opacity: 1, scale: 1 }}
          transition={{ delay: 0.3 }}
        >
          <span className="text-4xl font-mono font-bold text-white">
            {current.toFixed(1)}
          </span>
          <span className="text-lg text-[hsl(228,10%,50%)] ml-1">°{unit}</span>
        </motion.div>

        {/* Target label */}
        <motion.div
          className="flex items-center gap-1 mt-2"
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.5 }}
        >
          <span className="text-xs text-[hsl(228,10%,50%)]">Target:</span>
          <span className="text-sm font-mono font-medium text-white">
            {target.toFixed(1)}°{unit}
          </span>
        </motion.div>
      </div>
    </div>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Compact Gauge - for smaller spaces
// ─────────────────────────────────────────────────────────────────────────────

export interface CompactGaugeProps {
  current: number;
  target: number;
  isHeating?: boolean;
  className?: string;
}

export function CompactGauge({
  current,
  target,
  isHeating = false,
  className,
}: CompactGaugeProps) {
  return (
    <CircularGauge
      current={current}
      target={target}
      isHeating={isHeating}
      size={120}
      strokeWidth={8}
      showLabels={false}
      className={className}
    />
  );
}

export default CircularGauge;
