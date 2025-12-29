import { forwardRef, HTMLAttributes } from 'react';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const badgeVariants = cva(
  [
    'inline-flex items-center justify-center gap-1.5',
    'rounded-full font-medium',
    'transition-colors duration-200',
  ],
  {
    variants: {
      variant: {
        /** Default muted badge */
        default: [
          'bg-white/[0.06]',
          'text-[hsl(228,10%,70%)]',
          'border border-white/[0.08]',
        ],
        /** Primary accent badge */
        primary: [
          'bg-[hsl(187,100%,50%,0.15)]',
          'text-[hsl(187,100%,60%)]',
          'border border-[hsl(187,100%,50%,0.3)]',
        ],
        /** Success badge */
        success: [
          'bg-[hsl(160,84%,39%,0.15)]',
          'text-[hsl(160,84%,55%)]',
          'border border-[hsl(160,84%,39%,0.3)]',
        ],
        /** Warning badge */
        warning: [
          'bg-[hsl(38,92%,50%,0.15)]',
          'text-[hsl(38,92%,60%)]',
          'border border-[hsl(38,92%,50%,0.3)]',
        ],
        /** Error/danger badge */
        error: [
          'bg-[hsl(0,72%,51%,0.15)]',
          'text-[hsl(0,72%,60%)]',
          'border border-[hsl(0,72%,51%,0.3)]',
        ],
        /** Outline only */
        outline: [
          'bg-transparent',
          'text-white',
          'border border-white/[0.2]',
        ],
        /** Solid accent */
        solid: [
          'bg-[hsl(187,100%,50%)]',
          'text-[hsl(228,15%,4%)]',
          'font-semibold',
        ],
        /** Online status */
        online: [
          'bg-[hsl(160,84%,39%,0.2)]',
          'text-[hsl(160,84%,55%)]',
          'border border-[hsl(160,84%,39%,0.4)]',
        ],
        /** Offline status */
        offline: [
          'bg-white/[0.04]',
          'text-[hsl(228,10%,50%)]',
          'border border-white/[0.06]',
        ],
        /** Heating status (for Tado) */
        heating: [
          'bg-[hsl(25,95%,53%,0.15)]',
          'text-[hsl(25,95%,60%)]',
          'border border-[hsl(25,95%,53%,0.3)]',
        ],
      },
      size: {
        xs: 'h-5 px-1.5 text-[10px]',
        sm: 'h-6 px-2 text-xs',
        md: 'h-7 px-2.5 text-xs',
        lg: 'h-8 px-3 text-sm',
      },
      /** Add pulsing dot indicator */
      withDot: {
        true: '',
        false: '',
      },
    },
    defaultVariants: {
      variant: 'default',
      size: 'sm',
      withDot: false,
    },
  }
);

// Dot color mapping based on variant
const dotColorMap: Record<string, string> = {
  default: 'bg-[hsl(228,10%,50%)]',
  primary: 'bg-[hsl(187,100%,50%)]',
  success: 'bg-[hsl(160,84%,45%)]',
  warning: 'bg-[hsl(38,92%,50%)]',
  error: 'bg-[hsl(0,72%,51%)]',
  outline: 'bg-white',
  solid: 'bg-[hsl(228,15%,4%)]',
  online: 'bg-[hsl(160,84%,45%)]',
  offline: 'bg-[hsl(228,10%,40%)]',
  heating: 'bg-[hsl(25,95%,53%)]',
};

export interface BadgeProps
  extends HTMLAttributes<HTMLSpanElement>,
    VariantProps<typeof badgeVariants> {
  /** Icon to display before text */
  icon?: React.ReactNode;
}

const Badge = forwardRef<HTMLSpanElement, BadgeProps>(
  ({ className, variant = 'default', size, withDot, icon, children, ...props }, ref) => {
    const dotColor = dotColorMap[variant || 'default'];
    const shouldPulse = variant === 'online' || variant === 'heating';

    return (
      <span
        ref={ref}
        className={cn(badgeVariants({ variant, size, withDot }), className)}
        {...props}
      >
        {withDot && (
          <span className="relative flex h-2 w-2">
            {shouldPulse && (
              <span
                className={cn(
                  'animate-ping absolute inline-flex h-full w-full rounded-full opacity-75',
                  dotColor
                )}
              />
            )}
            <span
              className={cn(
                'relative inline-flex rounded-full h-2 w-2',
                dotColor
              )}
            />
          </span>
        )}
        {icon && <span className="shrink-0">{icon}</span>}
        {children}
      </span>
    );
  }
);

Badge.displayName = 'Badge';

// ─────────────────────────────────────────────────────────────────────────────
// Status Badge - Convenience component for device status
// ─────────────────────────────────────────────────────────────────────────────

export interface StatusBadgeProps extends Omit<BadgeProps, 'variant' | 'withDot'> {
  status: 'online' | 'offline' | 'warning' | 'heating';
}

const StatusBadge = forwardRef<HTMLSpanElement, StatusBadgeProps>(
  ({ status, children, ...props }, ref) => {
    const variantMap = {
      online: 'online',
      offline: 'offline',
      warning: 'warning',
      heating: 'heating',
    } as const;

    const labelMap = {
      online: 'Online',
      offline: 'Offline',
      warning: 'Warning',
      heating: 'Heating',
    };

    return (
      <Badge
        ref={ref}
        variant={variantMap[status]}
        withDot
        {...props}
      >
        {children || labelMap[status]}
      </Badge>
    );
  }
);

StatusBadge.displayName = 'StatusBadge';

// ─────────────────────────────────────────────────────────────────────────────
// CO2 Level Badge - Color-coded based on air quality
// ─────────────────────────────────────────────────────────────────────────────

export interface CO2BadgeProps extends Omit<BadgeProps, 'variant'> {
  value: number;
}

const CO2Badge = forwardRef<HTMLSpanElement, CO2BadgeProps>(
  ({ value, children, ...props }, ref) => {
    let variant: 'success' | 'primary' | 'warning' | 'error';
    let label: string;

    if (value < 600) {
      variant = 'success';
      label = 'Excellent';
    } else if (value < 1000) {
      variant = 'primary';
      label = 'Good';
    } else if (value < 1500) {
      variant = 'warning';
      label = 'Fair';
    } else {
      variant = 'error';
      label = 'Poor';
    }

    return (
      <Badge ref={ref} variant={variant} {...props}>
        {children || label}
      </Badge>
    );
  }
);

CO2Badge.displayName = 'CO2Badge';

export { Badge, StatusBadge, CO2Badge, badgeVariants };
