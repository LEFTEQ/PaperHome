import { forwardRef, useState, useCallback, useRef, useEffect } from 'react';
import { motion } from 'framer-motion';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const sliderVariants = cva(
  'relative w-full touch-none select-none',
  {
    variants: {
      size: {
        sm: 'h-1',
        md: 'h-1.5',
        lg: 'h-2',
      },
    },
    defaultVariants: {
      size: 'md',
    },
  }
);

const thumbVariants = cva(
  [
    'absolute top-1/2 -translate-y-1/2 -translate-x-1/2',
    'rounded-full bg-white',
    'shadow-[0_0_10px_rgba(0,0,0,0.3)]',
    'transition-transform duration-100',
    'hover:scale-110',
    'focus:outline-none focus:ring-2 focus:ring-offset-2 focus:ring-offset-bg-base',
    'cursor-grab active:cursor-grabbing active:scale-95',
  ],
  {
    variants: {
      size: {
        sm: 'h-3 w-3',
        md: 'h-4 w-4',
        lg: 'h-5 w-5',
      },
      variant: {
        default: 'focus:ring-accent',
        success: 'focus:ring-success',
        warning: 'focus:ring-warning',
      },
    },
    defaultVariants: {
      size: 'md',
      variant: 'default',
    },
  }
);

export interface SliderProps
  extends Omit<React.HTMLAttributes<HTMLDivElement>, 'onChange'>,
    VariantProps<typeof sliderVariants> {
  /** Current value (0-100 by default) */
  value: number;
  /** Callback when value changes */
  onChange?: (value: number) => void;
  /** Callback when drag ends */
  onChangeEnd?: (value: number) => void;
  /** Minimum value */
  min?: number;
  /** Maximum value */
  max?: number;
  /** Step increment */
  step?: number;
  /** Disable the slider */
  disabled?: boolean;
  /** Color variant */
  variant?: 'default' | 'success' | 'warning';
  /** Show value label */
  showValue?: boolean;
  /** Value label formatter */
  formatValue?: (value: number) => string;
  /** Label text */
  label?: string;
}

const Slider = forwardRef<HTMLDivElement, SliderProps>(
  (
    {
      className,
      value,
      onChange,
      onChangeEnd,
      min = 0,
      max = 100,
      step = 1,
      disabled = false,
      size,
      variant = 'default',
      showValue = false,
      formatValue = (v) => `${Math.round(v)}%`,
      label,
      ...props
    },
    ref
  ) => {
    const [isDragging, setIsDragging] = useState(false);
    const trackRef = useRef<HTMLDivElement>(null);

    const percentage = ((value - min) / (max - min)) * 100;

    const getValueFromPosition = useCallback(
      (clientX: number) => {
        if (!trackRef.current) return value;
        const rect = trackRef.current.getBoundingClientRect();
        const percent = Math.max(0, Math.min(1, (clientX - rect.left) / rect.width));
        const rawValue = min + percent * (max - min);
        const steppedValue = Math.round(rawValue / step) * step;
        return Math.max(min, Math.min(max, steppedValue));
      },
      [min, max, step, value]
    );

    const handlePointerDown = useCallback(
      (e: React.PointerEvent) => {
        if (disabled) return;
        e.preventDefault();
        setIsDragging(true);
        const newValue = getValueFromPosition(e.clientX);
        onChange?.(newValue);
        (e.target as HTMLElement).setPointerCapture(e.pointerId);
      },
      [disabled, getValueFromPosition, onChange]
    );

    const handlePointerMove = useCallback(
      (e: React.PointerEvent) => {
        if (!isDragging || disabled) return;
        const newValue = getValueFromPosition(e.clientX);
        onChange?.(newValue);
      },
      [isDragging, disabled, getValueFromPosition, onChange]
    );

    const handlePointerUp = useCallback(
      (e: React.PointerEvent) => {
        if (!isDragging) return;
        setIsDragging(false);
        (e.target as HTMLElement).releasePointerCapture(e.pointerId);
        onChangeEnd?.(value);
      },
      [isDragging, onChangeEnd, value]
    );

    // Keyboard support
    const handleKeyDown = useCallback(
      (e: React.KeyboardEvent) => {
        if (disabled) return;
        let newValue = value;

        switch (e.key) {
          case 'ArrowRight':
          case 'ArrowUp':
            newValue = Math.min(max, value + step);
            break;
          case 'ArrowLeft':
          case 'ArrowDown':
            newValue = Math.max(min, value - step);
            break;
          case 'Home':
            newValue = min;
            break;
          case 'End':
            newValue = max;
            break;
          default:
            return;
        }

        e.preventDefault();
        onChange?.(newValue);
        onChangeEnd?.(newValue);
      },
      [disabled, value, min, max, step, onChange, onChangeEnd]
    );

    // Track colors based on variant
    const trackColors = {
      default: {
        fill: 'hsl(187, 100%, 50%)',
        glow: 'hsla(187, 100%, 50%, 0.3)',
      },
      success: {
        fill: 'hsl(160, 84%, 39%)',
        glow: 'hsla(160, 84%, 39%, 0.3)',
      },
      warning: {
        fill: 'hsl(38, 92%, 50%)',
        glow: 'hsla(38, 92%, 50%, 0.3)',
      },
    };

    const colors = trackColors[variant];

    return (
      <div className={cn('w-full', disabled && 'opacity-50')} ref={ref} {...props}>
        {(label || showValue) && (
          <div className="flex justify-between items-center mb-2">
            {label && (
              <span className="text-sm font-medium text-text-secondary">
                {label}
              </span>
            )}
            {showValue && (
              <span className="text-sm font-mono text-text-muted">
                {formatValue(value)}
              </span>
            )}
          </div>
        )}
        <div
          ref={trackRef}
          className={cn(
            sliderVariants({ size }),
            'rounded-full bg-white/[0.1] cursor-pointer',
            className
          )}
          onPointerDown={handlePointerDown}
          onPointerMove={handlePointerMove}
          onPointerUp={handlePointerUp}
          onPointerCancel={handlePointerUp}
        >
          {/* Filled track */}
          <motion.div
            className="absolute inset-y-0 left-0 rounded-full"
            style={{
              width: `${percentage}%`,
              backgroundColor: colors.fill,
              boxShadow: `0 0 10px ${colors.glow}`,
            }}
            initial={false}
            animate={{ width: `${percentage}%` }}
            transition={{ duration: 0.1 }}
          />

          {/* Thumb */}
          <motion.div
            className={cn(thumbVariants({ size, variant }))}
            style={{ left: `${percentage}%` }}
            tabIndex={disabled ? -1 : 0}
            role="slider"
            aria-valuemin={min}
            aria-valuemax={max}
            aria-valuenow={value}
            aria-disabled={disabled}
            onKeyDown={handleKeyDown}
            initial={false}
            animate={{
              left: `${percentage}%`,
              scale: isDragging ? 0.95 : 1,
            }}
            transition={{ duration: 0.1 }}
          />
        </div>
      </div>
    );
  }
);

Slider.displayName = 'Slider';

// ─────────────────────────────────────────────────────────────────────────────
// Brightness Slider - Specialized for Hue lights (0-100%)
// ─────────────────────────────────────────────────────────────────────────────

export interface BrightnessSliderProps extends Omit<SliderProps, 'min' | 'max' | 'formatValue' | 'variant'> {}

const BrightnessSlider = forwardRef<HTMLDivElement, BrightnessSliderProps>(
  ({ label = 'Brightness', ...props }, ref) => (
    <Slider
      ref={ref}
      min={0}
      max={100}
      variant="default"
      formatValue={(v) => `${Math.round(v)}%`}
      label={label}
      showValue
      {...props}
    />
  )
);

BrightnessSlider.displayName = 'BrightnessSlider';

// ─────────────────────────────────────────────────────────────────────────────
// Temperature Slider - Specialized for Tado (typically 5-25°C)
// ─────────────────────────────────────────────────────────────────────────────

export interface TemperatureSliderProps extends Omit<SliderProps, 'formatValue' | 'variant'> {
  unit?: 'C' | 'F';
}

const TemperatureSlider = forwardRef<HTMLDivElement, TemperatureSliderProps>(
  ({ min = 5, max = 25, step = 0.5, unit = 'C', label = 'Temperature', ...props }, ref) => (
    <Slider
      ref={ref}
      min={min}
      max={max}
      step={step}
      variant="warning"
      formatValue={(v) => `${v.toFixed(1)}°${unit}`}
      label={label}
      showValue
      {...props}
    />
  )
);

TemperatureSlider.displayName = 'TemperatureSlider';

export { Slider, BrightnessSlider, TemperatureSlider, sliderVariants };
