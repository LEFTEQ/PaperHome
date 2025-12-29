import { forwardRef, ComponentPropsWithoutRef } from 'react';
import * as SwitchPrimitive from '@radix-ui/react-switch';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';

const toggleVariants = cva(
  [
    'peer inline-flex shrink-0 cursor-pointer items-center rounded-full',
    'border-2 border-transparent',
    'transition-all duration-200',
    'focus-visible:outline-none focus-visible:ring-2',
    'focus-visible:ring-offset-2 focus-visible:ring-offset-[hsl(228,15%,4%)]',
    'disabled:cursor-not-allowed disabled:opacity-50',
  ],
  {
    variants: {
      variant: {
        /** Default accent toggle */
        default: [
          'data-[state=unchecked]:bg-white/[0.1]',
          'data-[state=checked]:bg-[hsl(187,100%,50%)]',
          'data-[state=checked]:shadow-[0_0_12px_hsla(187,100%,50%,0.4)]',
          'focus-visible:ring-[hsl(187,100%,50%)]',
        ],
        /** Success toggle */
        success: [
          'data-[state=unchecked]:bg-white/[0.1]',
          'data-[state=checked]:bg-[hsl(160,84%,39%)]',
          'data-[state=checked]:shadow-[0_0_12px_hsla(160,84%,39%,0.4)]',
          'focus-visible:ring-[hsl(160,84%,39%)]',
        ],
        /** Warning toggle */
        warning: [
          'data-[state=unchecked]:bg-white/[0.1]',
          'data-[state=checked]:bg-[hsl(38,92%,50%)]',
          'data-[state=checked]:shadow-[0_0_12px_hsla(38,92%,50%,0.4)]',
          'focus-visible:ring-[hsl(38,92%,50%)]',
        ],
        /** Subtle toggle - less prominent */
        subtle: [
          'data-[state=unchecked]:bg-white/[0.06]',
          'data-[state=checked]:bg-white/[0.2]',
          'focus-visible:ring-white/50',
        ],
      },
      size: {
        sm: 'h-5 w-9',
        md: 'h-6 w-11',
        lg: 'h-7 w-14',
      },
    },
    defaultVariants: {
      variant: 'default',
      size: 'md',
    },
  }
);

const thumbVariants = cva(
  [
    'pointer-events-none block rounded-full bg-white',
    'shadow-lg transition-transform duration-200',
    'data-[state=unchecked]:translate-x-0.5',
  ],
  {
    variants: {
      size: {
        sm: 'h-4 w-4 data-[state=checked]:translate-x-4',
        md: 'h-5 w-5 data-[state=checked]:translate-x-5',
        lg: 'h-6 w-6 data-[state=checked]:translate-x-7',
      },
    },
    defaultVariants: {
      size: 'md',
    },
  }
);

export interface ToggleProps
  extends ComponentPropsWithoutRef<typeof SwitchPrimitive.Root>,
    VariantProps<typeof toggleVariants> {
  /** Label text */
  label?: string;
  /** Description text */
  description?: string;
  /** Label position */
  labelPosition?: 'left' | 'right';
}

const Toggle = forwardRef<
  React.ElementRef<typeof SwitchPrimitive.Root>,
  ToggleProps
>(
  (
    {
      className,
      variant,
      size,
      label,
      description,
      labelPosition = 'left',
      ...props
    },
    ref
  ) => {
    const toggle = (
      <SwitchPrimitive.Root
        ref={ref}
        className={cn(toggleVariants({ variant, size }), className)}
        {...props}
      >
        <SwitchPrimitive.Thumb className={cn(thumbVariants({ size }))} />
      </SwitchPrimitive.Root>
    );

    if (!label) return toggle;

    return (
      <div className={cn(
        'flex items-center gap-3',
        labelPosition === 'right' && 'flex-row-reverse justify-end'
      )}>
        <div className="flex flex-col">
          <span className="text-sm font-medium text-white">{label}</span>
          {description && (
            <span className="text-xs text-[hsl(228,10%,50%)]">{description}</span>
          )}
        </div>
        {toggle}
      </div>
    );
  }
);

Toggle.displayName = 'Toggle';

// ─────────────────────────────────────────────────────────────────────────────
// Light Toggle - Specific toggle for Hue lights with on/off icon
// ─────────────────────────────────────────────────────────────────────────────

export interface LightToggleProps extends Omit<ToggleProps, 'variant'> {
  /** Light is on */
  isOn: boolean;
  /** Room/light name */
  roomName?: string;
}

const LightToggle = forwardRef<
  React.ElementRef<typeof SwitchPrimitive.Root>,
  LightToggleProps
>(({ isOn, roomName, onCheckedChange, ...props }, ref) => (
  <Toggle
    ref={ref}
    variant="default"
    checked={isOn}
    onCheckedChange={onCheckedChange}
    label={roomName}
    {...props}
  />
));

LightToggle.displayName = 'LightToggle';

export { Toggle, LightToggle, toggleVariants };
