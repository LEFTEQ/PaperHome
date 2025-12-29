import { HTMLAttributes } from 'react';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const statusIndicatorVariants = cva(
  'relative inline-flex rounded-full',
  {
    variants: {
      status: {
        online: [
          'bg-[hsl(160,84%,45%)]',
          'shadow-[0_0_8px_hsla(160,84%,45%,0.6)]',
        ],
        offline: 'bg-[hsl(228,10%,40%)]',
        warning: [
          'bg-[hsl(38,92%,50%)]',
          'shadow-[0_0_8px_hsla(38,92%,50%,0.6)]',
        ],
        error: [
          'bg-[hsl(0,72%,51%)]',
          'shadow-[0_0_8px_hsla(0,72%,51%,0.6)]',
        ],
        heating: [
          'bg-[hsl(25,95%,53%)]',
          'shadow-[0_0_8px_hsla(25,95%,53%,0.6)]',
        ],
        idle: 'bg-[hsl(228,10%,50%)]',
        connecting: [
          'bg-[hsl(187,100%,50%)]',
          'shadow-[0_0_8px_hsla(187,100%,50%,0.6)]',
        ],
      },
      size: {
        xs: 'h-1.5 w-1.5',
        sm: 'h-2 w-2',
        md: 'h-2.5 w-2.5',
        lg: 'h-3 w-3',
        xl: 'h-4 w-4',
      },
      pulse: {
        true: '',
        false: '',
      },
    },
    defaultVariants: {
      status: 'offline',
      size: 'md',
      pulse: true,
    },
  }
);

// Pulse ring color mapping
const pulseColorMap: Record<string, string> = {
  online: 'bg-[hsl(160,84%,45%)]',
  offline: '',
  warning: 'bg-[hsl(38,92%,50%)]',
  error: 'bg-[hsl(0,72%,51%)]',
  heating: 'bg-[hsl(25,95%,53%)]',
  idle: '',
  connecting: 'bg-[hsl(187,100%,50%)]',
};

export interface StatusIndicatorProps
  extends HTMLAttributes<HTMLSpanElement>,
    VariantProps<typeof statusIndicatorVariants> {
  /** Show label text next to indicator */
  label?: string;
  /** Position of label */
  labelPosition?: 'left' | 'right';
}

export function StatusIndicator({
  className,
  status = 'offline',
  size,
  pulse = true,
  label,
  labelPosition = 'right',
  ...props
}: StatusIndicatorProps) {
  const shouldPulse = pulse && status !== 'offline' && status !== 'idle';
  const pulseColor = pulseColorMap[status || 'offline'];

  const indicator = (
    <span
      className={cn(
        statusIndicatorVariants({ status, size, pulse }),
        className
      )}
      {...props}
    >
      {shouldPulse && pulseColor && (
        <span
          className={cn(
            'absolute inset-0 rounded-full animate-ping opacity-75',
            pulseColor
          )}
        />
      )}
    </span>
  );

  if (!label) return indicator;

  const labelClass = cn(
    'text-xs text-[hsl(228,10%,60%)]',
    labelPosition === 'left' ? 'mr-1.5' : 'ml-1.5'
  );

  return (
    <span className="inline-flex items-center">
      {labelPosition === 'left' && <span className={labelClass}>{label}</span>}
      {indicator}
      {labelPosition === 'right' && <span className={labelClass}>{label}</span>}
    </span>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Device Status Indicator - with automatic labeling
// ─────────────────────────────────────────────────────────────────────────────

export interface DeviceStatusProps extends Omit<StatusIndicatorProps, 'status' | 'label'> {
  isOnline: boolean;
  showLabel?: boolean;
}

export function DeviceStatus({
  isOnline,
  showLabel = true,
  ...props
}: DeviceStatusProps) {
  return (
    <StatusIndicator
      status={isOnline ? 'online' : 'offline'}
      label={showLabel ? (isOnline ? 'Online' : 'Offline') : undefined}
      {...props}
    />
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Heating Status Indicator - for Tado
// ─────────────────────────────────────────────────────────────────────────────

export interface HeatingStatusProps extends Omit<StatusIndicatorProps, 'status' | 'label'> {
  isHeating: boolean;
  showLabel?: boolean;
}

export function HeatingStatus({
  isHeating,
  showLabel = true,
  ...props
}: HeatingStatusProps) {
  return (
    <StatusIndicator
      status={isHeating ? 'heating' : 'idle'}
      label={showLabel ? (isHeating ? 'Heating' : 'Idle') : undefined}
      {...props}
    />
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Connection Status Indicator - for WebSocket/MQTT
// ─────────────────────────────────────────────────────────────────────────────

export interface ConnectionStatusProps extends Omit<StatusIndicatorProps, 'status' | 'label'> {
  state: 'connected' | 'connecting' | 'disconnected' | 'error';
  showLabel?: boolean;
}

export function ConnectionStatus({
  state,
  showLabel = true,
  ...props
}: ConnectionStatusProps) {
  const statusMap = {
    connected: 'online',
    connecting: 'connecting',
    disconnected: 'offline',
    error: 'error',
  } as const;

  const labelMap = {
    connected: 'Connected',
    connecting: 'Connecting',
    disconnected: 'Disconnected',
    error: 'Error',
  };

  return (
    <StatusIndicator
      status={statusMap[state]}
      label={showLabel ? labelMap[state] : undefined}
      {...props}
    />
  );
}

export { statusIndicatorVariants };
