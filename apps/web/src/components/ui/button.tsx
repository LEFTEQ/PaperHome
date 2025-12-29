import { forwardRef, ButtonHTMLAttributes } from 'react';
import { motion, HTMLMotionProps } from 'framer-motion';
import { cva, type VariantProps } from 'class-variance-authority';
import { Loader2 } from 'lucide-react';
import { cn } from '@/lib/utils';
import { buttonTap } from '@/lib/animations';

const buttonVariants = cva(
  [
    'relative inline-flex items-center justify-center gap-2',
    'font-medium transition-all duration-200',
    'focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-offset-2 focus-visible:ring-offset-bg-base',
    'disabled:pointer-events-none disabled:opacity-50',
    'active:scale-[0.98]',
  ],
  {
    variants: {
      variant: {
        /** Primary accent button with glow */
        primary: [
          'bg-gradient-to-r from-accent to-accent/80',
          'text-bg-base font-semibold',
          'shadow-[0_0_20px_rgb(0_229_255/0.3)]',
          'hover:shadow-[0_0_30px_rgb(0_229_255/0.4)]',
          'hover:from-accent-hover hover:to-accent',
          'focus-visible:ring-accent',
        ],
        /** Secondary glass button */
        secondary: [
          'bg-glass-bright backdrop-blur-sm',
          'border border-glass-border-hover',
          'text-white',
          'hover:bg-white/[0.1] hover:border-glass-border-bright',
          'focus-visible:ring-white/50',
        ],
        /** Ghost button - minimal */
        ghost: [
          'bg-transparent',
          'text-text-secondary',
          'hover:bg-glass-hover hover:text-white',
          'focus-visible:ring-white/30',
        ],
        /** Outline button - bordered */
        outline: [
          'bg-transparent',
          'border border-glass-border-hover',
          'text-white',
          'hover:bg-glass-hover hover:border-white/[0.2]',
          'focus-visible:ring-white/30',
        ],
        /** Destructive - danger actions */
        destructive: [
          'bg-error',
          'text-white font-semibold',
          'shadow-[0_0_20px_rgb(239_68_68/0.3)]',
          'hover:bg-error/90',
          'hover:shadow-[0_0_30px_rgb(239_68_68/0.4)]',
          'focus-visible:ring-error',
        ],
        /** Success - confirmation actions */
        success: [
          'bg-success',
          'text-white font-semibold',
          'shadow-[0_0_20px_rgb(16_185_129/0.3)]',
          'hover:bg-success/90',
          'hover:shadow-[0_0_30px_rgb(16_185_129/0.4)]',
          'focus-visible:ring-success',
        ],
        /** Glass with accent border */
        glassAccent: [
          'bg-glass backdrop-blur-sm',
          'border border-accent/30',
          'text-accent',
          'hover:bg-accent-subtle hover:border-accent/50',
          'focus-visible:ring-accent',
        ],
      },
      size: {
        xs: 'h-7 px-2.5 text-xs rounded-md',
        sm: 'h-8 px-3 text-sm rounded-lg',
        md: 'h-10 px-4 text-sm rounded-lg',
        lg: 'h-12 px-6 text-base rounded-xl',
        xl: 'h-14 px-8 text-lg rounded-xl',
        icon: 'h-10 w-10 rounded-lg',
        'icon-sm': 'h-8 w-8 rounded-md',
        'icon-lg': 'h-12 w-12 rounded-xl',
      },
      fullWidth: {
        true: 'w-full',
        false: '',
      },
    },
    defaultVariants: {
      variant: 'primary',
      size: 'md',
      fullWidth: false,
    },
  }
);

export interface ButtonProps
  extends Omit<ButtonHTMLAttributes<HTMLButtonElement>, 'onAnimationStart' | 'onAnimationEnd' | 'onDrag' | 'onDragEnd' | 'onDragStart'>,
    VariantProps<typeof buttonVariants> {
  /** Show loading spinner */
  isLoading?: boolean;
  /** Loading text to display */
  loadingText?: string;
  /** Icon to display before text */
  leftIcon?: React.ReactNode;
  /** Icon to display after text */
  rightIcon?: React.ReactNode;
  /** Enable motion animations */
  animate?: boolean;
}

const Button = forwardRef<HTMLButtonElement, ButtonProps>(
  (
    {
      className,
      variant,
      size,
      fullWidth,
      isLoading = false,
      loadingText,
      leftIcon,
      rightIcon,
      animate = true,
      disabled,
      children,
      ...props
    },
    ref
  ) => {
    const isDisabled = disabled || isLoading;
    const buttonClassName = cn(buttonVariants({ variant, size, fullWidth }), className);

    const content = (
      <>
        {isLoading ? (
          <>
            <Loader2 className="h-4 w-4 animate-spin" />
            {loadingText && <span>{loadingText}</span>}
          </>
        ) : (
          <>
            {leftIcon && <span className="shrink-0">{leftIcon}</span>}
            {children}
            {rightIcon && <span className="shrink-0">{rightIcon}</span>}
          </>
        )}
      </>
    );

    if (animate) {
      return (
        <motion.button
          ref={ref}
          disabled={isDisabled}
          className={buttonClassName}
          whileTap={!isDisabled ? buttonTap.tap : undefined}
          {...(props as HTMLMotionProps<'button'>)}
        >
          {content}
        </motion.button>
      );
    }

    return (
      <button ref={ref} disabled={isDisabled} className={buttonClassName} {...props}>
        {content}
      </button>
    );
  }
);

Button.displayName = 'Button';

// ─────────────────────────────────────────────────────────────────────────────
// Icon Button variant
// ─────────────────────────────────────────────────────────────────────────────

export interface IconButtonProps extends Omit<ButtonProps, 'leftIcon' | 'rightIcon'> {
  /** Accessible label for the button */
  'aria-label': string;
}

const IconButton = forwardRef<HTMLButtonElement, IconButtonProps>(
  ({ size = 'icon', children, ...props }, ref) => (
    <Button ref={ref} size={size} {...props}>
      {children}
    </Button>
  )
);

IconButton.displayName = 'IconButton';

export { Button, IconButton, buttonVariants };
