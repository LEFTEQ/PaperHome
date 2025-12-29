import { forwardRef, HTMLAttributes } from 'react';
import { motion, HTMLMotionProps } from 'framer-motion';
import { cva, type VariantProps } from 'class-variance-authority';
import { cn } from '@/lib/utils';
import { cardHover, glowHover } from '@/lib/animations';

const glassCardVariants = cva(
  [
    'relative rounded-2xl border backdrop-blur-xl',
    'bg-glass border-glass-border',
    'transition-all duration-200',
  ],
  {
    variants: {
      variant: {
        default: '',
        elevated: 'bg-glass-hover border-glass-bright',
        solid: 'bg-bg-surface backdrop-blur-none',
      },
      size: {
        sm: 'p-4',
        md: 'p-5',
        lg: 'p-6',
        xl: 'p-8',
        none: '',
      },
      interactive: {
        true: [
          'cursor-pointer',
          'hover:bg-glass-hover hover:border-glass-border-hover',
          'hover:shadow-lg hover:-translate-y-0.5',
          'active:scale-[0.99]',
        ],
        false: '',
      },
      glow: {
        true: 'shadow-[0_0_30px_rgb(0_229_255/0.15)]',
        accent: 'shadow-[0_0_30px_rgb(0_229_255/0.15)] hover:shadow-[0_0_50px_rgb(0_229_255/0.25)]',
        success: 'shadow-[0_0_20px_rgb(16_185_129/0.2)]',
        warning: 'shadow-[0_0_20px_rgb(245_158_11/0.2)]',
        error: 'shadow-[0_0_20px_rgb(239_68_68/0.2)]',
        false: '',
      },
      border: {
        default: '',
        gradient: [
          'border-0',
          'before:absolute before:inset-0 before:rounded-2xl before:p-px',
          'before:bg-gradient-to-br before:from-white/10 before:to-transparent',
          'before:-z-10',
        ],
        accent: 'border-accent/30',
      },
    },
    defaultVariants: {
      variant: 'default',
      size: 'md',
      interactive: false,
      glow: false,
      border: 'default',
    },
  }
);

export interface GlassCardProps
  extends Omit<HTMLAttributes<HTMLDivElement>, 'onAnimationStart' | 'onAnimationEnd' | 'onDrag' | 'onDragEnd' | 'onDragStart'>,
    VariantProps<typeof glassCardVariants> {
  /** Enable motion animations */
  animate?: boolean;
  /** Custom motion variants */
  motionVariants?: typeof cardHover;
}

const GlassCard = forwardRef<HTMLDivElement, GlassCardProps>(
  (
    {
      className,
      variant,
      size,
      interactive,
      glow,
      border,
      animate = false,
      motionVariants,
      children,
      ...props
    },
    ref
  ) => {
    const baseClassName = cn(
      glassCardVariants({ variant, size, interactive, glow, border }),
      className
    );

    if (animate || motionVariants) {
      return (
        <motion.div
          ref={ref}
          className={baseClassName}
          variants={motionVariants || (interactive ? cardHover : undefined)}
          initial="rest"
          whileHover={interactive ? 'hover' : undefined}
          whileTap={interactive ? 'tap' : undefined}
          {...(props as HTMLMotionProps<'div'>)}
        >
          {children}
        </motion.div>
      );
    }

    return (
      <div ref={ref} className={baseClassName} {...props}>
        {children}
      </div>
    );
  }
);
GlassCard.displayName = 'GlassCard';

// ─────────────────────────────────────────────────────────────────────────────
// Sub-components for structured content
// ─────────────────────────────────────────────────────────────────────────────

const GlassCardHeader = forwardRef<HTMLDivElement, HTMLAttributes<HTMLDivElement>>(
  ({ className, ...props }, ref) => (
    <div
      ref={ref}
      className={cn('flex flex-col gap-1.5 pb-4', className)}
      {...props}
    />
  )
);
GlassCardHeader.displayName = 'GlassCardHeader';

const GlassCardTitle = forwardRef<HTMLHeadingElement, HTMLAttributes<HTMLHeadingElement>>(
  ({ className, ...props }, ref) => (
    <h3
      ref={ref}
      className={cn(
        'font-semibold text-lg leading-tight tracking-tight text-white',
        className
      )}
      {...props}
    />
  )
);
GlassCardTitle.displayName = 'GlassCardTitle';

const GlassCardDescription = forwardRef<
  HTMLParagraphElement,
  HTMLAttributes<HTMLParagraphElement>
>(({ className, ...props }, ref) => (
  <p
    ref={ref}
    className={cn('text-sm text-text-muted', className)}
    {...props}
  />
));
GlassCardDescription.displayName = 'GlassCardDescription';

const GlassCardContent = forwardRef<HTMLDivElement, HTMLAttributes<HTMLDivElement>>(
  ({ className, ...props }, ref) => (
    <div ref={ref} className={cn('', className)} {...props} />
  )
);
GlassCardContent.displayName = 'GlassCardContent';

const GlassCardFooter = forwardRef<HTMLDivElement, HTMLAttributes<HTMLDivElement>>(
  ({ className, ...props }, ref) => (
    <div
      ref={ref}
      className={cn('flex items-center pt-4 mt-auto', className)}
      {...props}
    />
  )
);
GlassCardFooter.displayName = 'GlassCardFooter';

// ─────────────────────────────────────────────────────────────────────────────
// Bento Grid Card Variants
// ─────────────────────────────────────────────────────────────────────────────

const bentoSizeMap = {
  '1x1': 'bento-1x1',
  '2x1': 'bento-2x1',
  '1x2': 'bento-1x2',
  '2x2': 'bento-2x2',
} as const;

export interface BentoCardProps extends GlassCardProps {
  bentoSize?: keyof typeof bentoSizeMap;
}

const BentoCard = forwardRef<HTMLDivElement, BentoCardProps>(
  ({ className, bentoSize = '1x1', ...props }, ref) => (
    <GlassCard
      ref={ref}
      className={cn(bentoSizeMap[bentoSize], 'flex flex-col', className)}
      {...props}
    />
  )
);
BentoCard.displayName = 'BentoCard';

export {
  GlassCard,
  GlassCardHeader,
  GlassCardTitle,
  GlassCardDescription,
  GlassCardContent,
  GlassCardFooter,
  BentoCard,
  glassCardVariants,
};
