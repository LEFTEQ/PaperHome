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
    'focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-offset-2 focus-visible:ring-offset-[hsl(228,15%,4%)]',
    'disabled:pointer-events-none disabled:opacity-50',
    'active:scale-[0.98]',
  ],
  {
    variants: {
      variant: {
        /** Primary accent button with glow */
        primary: [
          'bg-gradient-to-r from-[hsl(187,100%,50%)] to-[hsl(187,100%,40%)]',
          'text-[hsl(228,15%,4%)] font-semibold',
          'shadow-[0_0_20px_hsla(187,100%,50%,0.3)]',
          'hover:shadow-[0_0_30px_hsla(187,100%,50%,0.4)]',
          'hover:from-[hsl(187,100%,55%)] hover:to-[hsl(187,100%,45%)]',
          'focus-visible:ring-[hsl(187,100%,50%)]',
        ],
        /** Secondary glass button */
        secondary: [
          'bg-white/[0.06] backdrop-blur-sm',
          'border border-white/[0.1]',
          'text-white',
          'hover:bg-white/[0.1] hover:border-white/[0.15]',
          'focus-visible:ring-white/50',
        ],
        /** Ghost button - minimal */
        ghost: [
          'bg-transparent',
          'text-[hsl(228,10%,70%)]',
          'hover:bg-white/[0.05] hover:text-white',
          'focus-visible:ring-white/30',
        ],
        /** Outline button - bordered */
        outline: [
          'bg-transparent',
          'border border-white/[0.1]',
          'text-white',
          'hover:bg-white/[0.05] hover:border-white/[0.2]',
          'focus-visible:ring-white/30',
        ],
        /** Destructive - danger actions */
        destructive: [
          'bg-[hsl(0,72%,51%)]',
          'text-white font-semibold',
          'shadow-[0_0_20px_hsla(0,72%,51%,0.3)]',
          'hover:bg-[hsl(0,72%,45%)]',
          'hover:shadow-[0_0_30px_hsla(0,72%,51%,0.4)]',
          'focus-visible:ring-[hsl(0,72%,51%)]',
        ],
        /** Success - confirmation actions */
        success: [
          'bg-[hsl(160,84%,39%)]',
          'text-white font-semibold',
          'shadow-[0_0_20px_hsla(160,84%,39%,0.3)]',
          'hover:bg-[hsl(160,84%,35%)]',
          'hover:shadow-[0_0_30px_hsla(160,84%,39%,0.4)]',
          'focus-visible:ring-[hsl(160,84%,39%)]',
        ],
        /** Glass with accent border */
        glassAccent: [
          'bg-white/[0.03] backdrop-blur-sm',
          'border border-[hsl(187,100%,50%,0.3)]',
          'text-[hsl(187,100%,50%)]',
          'hover:bg-[hsl(187,100%,50%,0.1)] hover:border-[hsl(187,100%,50%,0.5)]',
          'focus-visible:ring-[hsl(187,100%,50%)]',
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
