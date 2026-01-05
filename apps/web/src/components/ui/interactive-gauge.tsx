import { useState, useRef, useCallback, useEffect, useMemo } from 'react';
import { motion, useMotionValue, useSpring, useTransform } from 'framer-motion';
import { Flame, Zap, AlertTriangle } from 'lucide-react';
import { cn } from '@/lib/utils';

export interface InteractiveGaugeProps {
  /** Current temperature from ESP32 sensor */
  currentTemp: number;
  /** Target temperature (user's desired temp) */
  targetTemp: number;
  /** Current Tado target (what's set on the thermostat) */
  tadoTarget?: number;
  /** Minimum temperature */
  min?: number;
  /** Maximum temperature */
  max?: number;
  /** Temperature unit */
  unit?: 'C' | 'F';
  /** Is heating active */
  isHeating?: boolean;
  /** Is auto-adjust enabled */
  isAutoAdjust?: boolean;
  /** Is device offline */
  isOffline?: boolean;
  /** Size of the gauge */
  size?: number;
  /** Stroke width of the arc */
  strokeWidth?: number;
  /** Callback when target is changed */
  onTargetChange?: (value: number) => void;
  /** Temperature step (default 0.5) */
  step?: number;
  /** Show labels */
  showLabels?: boolean;
  /** Additional className */
  className?: string;
}

export function InteractiveGauge({
  currentTemp,
  targetTemp,
  tadoTarget,
  min = 5,
  max = 25,
  unit = 'C',
  isHeating = false,
  isAutoAdjust = false,
  isOffline = false,
  size = 200,
  strokeWidth = 12,
  onTargetChange,
  step = 0.5,
  showLabels = true,
  className,
}: InteractiveGaugeProps) {
  const svgRef = useRef<SVGSVGElement>(null);
  const [isDragging, setIsDragging] = useState(false);
  const [previewTarget, setPreviewTarget] = useState<number | null>(null);

  const radius = (size - strokeWidth) / 2;
  const center = size / 2;

  // Arc configuration - 270 degree arc
  const startAngle = 135;
  const endAngle = 405;
  const totalAngle = endAngle - startAngle;

  // Motion values for smooth animation
  const targetMotion = useMotionValue(targetTemp);
  const targetSpring = useSpring(targetMotion, {
    stiffness: 300,
    damping: 30,
  });

  // Update motion value when targetTemp changes
  useEffect(() => {
    targetMotion.set(targetTemp);
  }, [targetTemp, targetMotion]);

  // Convert value to angle
  const valueToAngle = useCallback(
    (value: number) => {
      const percent = (value - min) / (max - min);
      return startAngle + percent * totalAngle;
    },
    [min, max, startAngle, totalAngle]
  );

  // Convert angle to value (snapped to step)
  const angleToValue = useCallback(
    (angle: number) => {
      // Normalize angle to 0-totalAngle range
      let normalizedAngle = angle - startAngle;
      if (normalizedAngle < 0) normalizedAngle += 360;
      if (normalizedAngle > totalAngle) normalizedAngle = totalAngle;

      const percent = Math.max(0, Math.min(1, normalizedAngle / totalAngle));
      const rawValue = min + percent * (max - min);

      // Snap to step
      return Math.round(rawValue / step) * step;
    },
    [min, max, startAngle, totalAngle, step]
  );

  // Convert cartesian to polar angle
  const cartesianToAngle = useCallback(
    (x: number, y: number) => {
      const dx = x - center;
      const dy = y - center;
      let angle = (Math.atan2(dy, dx) * 180) / Math.PI + 90;
      if (angle < 0) angle += 360;
      return angle;
    },
    [center]
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
        'M', start.x, start.y,
        'A', radius, radius, 0, largeArcFlag, 0, end.x, end.y,
      ].join(' ');
    },
    [polarToCartesian, radius]
  );

  // Paths
  const backgroundPath = useMemo(
    () => describeArc(startAngle, endAngle),
    [describeArc, startAngle, endAngle]
  );

  const currentAngle = valueToAngle(Math.max(min, Math.min(max, currentTemp)));
  const targetAngle = valueToAngle(Math.max(min, Math.min(max, previewTarget ?? targetTemp)));

  const currentPath = useMemo(
    () => describeArc(startAngle, currentAngle),
    [describeArc, startAngle, currentAngle]
  );

  // Target indicator position
  const targetPos = polarToCartesian(targetAngle);

  // Handle pointer events for dragging
  const handlePointerDown = useCallback(
    (e: React.PointerEvent) => {
      if (isOffline || !onTargetChange) return;

      const svg = svgRef.current;
      if (!svg) return;

      e.preventDefault();
      (e.target as Element).setPointerCapture(e.pointerId);
      setIsDragging(true);

      const rect = svg.getBoundingClientRect();
      const x = ((e.clientX - rect.left) / rect.width) * size;
      const y = ((e.clientY - rect.top) / rect.height) * size;
      const angle = cartesianToAngle(x, y);
      const value = angleToValue(angle);

      setPreviewTarget(value);
    },
    [isOffline, onTargetChange, size, cartesianToAngle, angleToValue]
  );

  const handlePointerMove = useCallback(
    (e: React.PointerEvent) => {
      if (!isDragging) return;

      const svg = svgRef.current;
      if (!svg) return;

      const rect = svg.getBoundingClientRect();
      const x = ((e.clientX - rect.left) / rect.width) * size;
      const y = ((e.clientY - rect.top) / rect.height) * size;
      const angle = cartesianToAngle(x, y);
      const value = angleToValue(angle);

      setPreviewTarget(value);
    },
    [isDragging, size, cartesianToAngle, angleToValue]
  );

  const handlePointerUp = useCallback(
    (e: React.PointerEvent) => {
      if (!isDragging) return;

      (e.target as Element).releasePointerCapture(e.pointerId);
      setIsDragging(false);

      if (previewTarget !== null && onTargetChange) {
        onTargetChange(previewTarget);
      }
      setPreviewTarget(null);
    },
    [isDragging, previewTarget, onTargetChange]
  );

  // Colors based on state
  const getColors = () => {
    if (isOffline) {
      return {
        current: 'rgba(255, 255, 255, 0.3)',
        target: 'rgba(255, 255, 255, 0.5)',
        glow: 'transparent',
      };
    }
    if (isHeating) {
      return {
        current: 'hsl(25, 95%, 53%)',
        target: 'hsl(25, 95%, 60%)',
        glow: 'hsla(25, 95%, 53%, 0.4)',
      };
    }
    if (currentTemp >= targetTemp) {
      return {
        current: 'hsl(160, 84%, 45%)',
        target: 'hsl(160, 84%, 55%)',
        glow: 'hsla(160, 84%, 45%, 0.3)',
      };
    }
    return {
      current: 'hsl(187, 100%, 50%)',
      target: 'hsl(187, 100%, 60%)',
      glow: 'hsla(187, 100%, 50%, 0.3)',
    };
  };

  const colors = getColors();

  // Generate tick marks
  const ticks = useMemo(() => {
    const tickMarks = [];
    const tickStep = 5;
    for (let temp = min; temp <= max; temp += tickStep) {
      const angle = valueToAngle(temp);
      const pos = polarToCartesian(angle);
      const innerPos = {
        x: center + (radius - 15) * Math.cos(((angle - 90) * Math.PI) / 180),
        y: center + (radius - 15) * Math.sin(((angle - 90) * Math.PI) / 180),
      };
      const labelPos = {
        x: center + (radius - 30) * Math.cos(((angle - 90) * Math.PI) / 180),
        y: center + (radius - 30) * Math.sin(((angle - 90) * Math.PI) / 180),
      };
      tickMarks.push({ temp, outerPos: pos, innerPos, labelPos });
    }
    return tickMarks;
  }, [min, max, center, radius, valueToAngle, polarToCartesian]);

  const displayTarget = previewTarget ?? targetTemp;

  return (
    <div className={cn('relative inline-flex flex-col items-center', className)}>
      {/* Heat gradient background when heating */}
      {isHeating && !isOffline && (
        <div
          className="absolute inset-0 rounded-full opacity-20 animate-pulse"
          style={{
            background: `radial-gradient(circle, hsla(25, 95%, 53%, 0.3) 0%, transparent 70%)`,
          }}
        />
      )}

      <svg
        ref={svgRef}
        width={size}
        height={size}
        viewBox={`0 0 ${size} ${size}`}
        className={cn(
          'overflow-visible touch-none',
          onTargetChange && !isOffline && 'cursor-pointer'
        )}
        onPointerDown={handlePointerDown}
        onPointerMove={handlePointerMove}
        onPointerUp={handlePointerUp}
        onPointerCancel={handlePointerUp}
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
          stroke={colors.current}
          strokeWidth={strokeWidth}
          strokeLinecap="round"
          initial={{ pathLength: 0 }}
          animate={{ pathLength: 1 }}
          transition={{ duration: 1, ease: 'easeOut' }}
          style={{
            filter: `drop-shadow(0 0 10px ${colors.glow})`,
          }}
        />

        {/* Tick marks */}
        {showLabels && ticks.map(({ temp, outerPos, innerPos, labelPos }) => (
          <g key={temp}>
            <line
              x1={outerPos.x}
              y1={outerPos.y}
              x2={innerPos.x}
              y2={innerPos.y}
              stroke="rgba(255, 255, 255, 0.1)"
              strokeWidth={1}
            />
            <text
              x={labelPos.x}
              y={labelPos.y}
              fill="rgba(255, 255, 255, 0.3)"
              fontSize={10}
              textAnchor="middle"
              dominantBaseline="middle"
              className="font-mono select-none"
            >
              {temp}
            </text>
          </g>
        ))}

        {/* Target indicator (draggable) */}
        <motion.circle
          cx={targetPos.x}
          cy={targetPos.y}
          r={isDragging ? 12 : 8}
          fill="white"
          stroke={colors.target}
          strokeWidth={3}
          initial={{ scale: 0 }}
          animate={{
            scale: 1,
            r: isDragging ? 12 : 8,
          }}
          transition={{
            scale: { delay: 0.5, type: 'spring', stiffness: 400, damping: 20 },
            r: { duration: 0.15 },
          }}
          style={{
            filter: `drop-shadow(0 0 8px ${colors.glow})`,
            cursor: onTargetChange && !isOffline ? 'grab' : 'default',
          }}
        />

        {/* Tado target indicator (if different from user target) */}
        {tadoTarget !== undefined && Math.abs(tadoTarget - targetTemp) > 0.1 && (
          <motion.circle
            cx={polarToCartesian(valueToAngle(tadoTarget)).x}
            cy={polarToCartesian(valueToAngle(tadoTarget)).y}
            r={4}
            fill="transparent"
            stroke="rgba(255, 255, 255, 0.4)"
            strokeWidth={2}
            strokeDasharray="3 2"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
          />
        )}
      </svg>

      {/* Center content */}
      <div className="absolute inset-0 flex flex-col items-center justify-center pointer-events-none">
        {/* Status badge */}
        {(isHeating || isAutoAdjust || isOffline) && (
          <motion.div
            className="flex items-center gap-1 mb-1"
            initial={{ opacity: 0, y: 5 }}
            animate={{ opacity: 1, y: 0 }}
          >
            {isOffline ? (
              <>
                <AlertTriangle className="h-4 w-4 text-warning" />
                <span className="text-xs font-medium text-warning">Offline</span>
              </>
            ) : isHeating ? (
              <>
                <Flame className="h-4 w-4 text-heating" />
                <span className="text-xs font-medium text-heating">Heating</span>
              </>
            ) : isAutoAdjust ? (
              <>
                <Zap className="h-4 w-4 text-accent" />
                <span className="text-xs font-medium text-accent">Auto</span>
              </>
            ) : null}
          </motion.div>
        )}

        {/* Current temperature */}
        <motion.div
          className="flex items-baseline"
          initial={{ opacity: 0, scale: 0.9 }}
          animate={{ opacity: 1, scale: 1 }}
          transition={{ delay: 0.3 }}
        >
          <span className={cn(
            'text-4xl font-mono font-bold',
            isOffline ? 'text-text-muted' : 'text-white'
          )}>
            {currentTemp.toFixed(1)}
          </span>
          <span className="text-lg text-text-muted ml-1">°{unit}</span>
        </motion.div>

        {/* Target label */}
        <motion.div
          className="flex items-center gap-1 mt-2"
          initial={{ opacity: 0 }}
          animate={{ opacity: 1 }}
          transition={{ delay: 0.5 }}
        >
          <span className="text-xs text-text-muted">Target:</span>
          <span className={cn(
            'text-sm font-mono font-medium',
            isDragging ? 'text-accent' : 'text-white'
          )}>
            {displayTarget.toFixed(1)}°{unit}
          </span>
        </motion.div>

        {/* ESP32 vs Tado comparison (when auto-adjust is enabled) */}
        {isAutoAdjust && tadoTarget !== undefined && (
          <motion.div
            className="flex flex-col items-center gap-0.5 mt-2 text-xs text-text-subtle"
            initial={{ opacity: 0 }}
            animate={{ opacity: 1 }}
            transition={{ delay: 0.7 }}
          >
            <span>Tado: {tadoTarget.toFixed(1)}°{unit}</span>
          </motion.div>
        )}
      </div>
    </div>
  );
}

export default InteractiveGauge;
