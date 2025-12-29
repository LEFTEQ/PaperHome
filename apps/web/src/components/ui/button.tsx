import { forwardRef, ButtonHTMLAttributes } from 'react';
import { cn } from '@/lib/utils';

export interface ButtonProps extends ButtonHTMLAttributes<HTMLButtonElement> {
  variant?: 'primary' | 'secondary' | 'ghost' | 'outline' | 'destructive';
  size?: 'sm' | 'md' | 'lg';
}

const Button = forwardRef<HTMLButtonElement, ButtonProps>(
  ({ className, variant = 'primary', size = 'md', children, disabled, ...props }, ref) => {
    return (
      <button
        ref={ref}
        disabled={disabled}
        className={cn(
          // Base styles
          'relative inline-flex items-center justify-center font-medium transition-all duration-200 ease-out',
          'focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-offset-2',
          'disabled:pointer-events-none disabled:opacity-50',
          // Subtle paper press effect
          'active:translate-y-[1px] active:shadow-none',

          // Size variants
          {
            'h-8 px-3 text-sm rounded-md gap-1.5': size === 'sm',
            'h-10 px-4 text-sm rounded-lg gap-2': size === 'md',
            'h-12 px-6 text-base rounded-lg gap-2.5': size === 'lg',
          },

          // Color variants
          {
            // Primary - terracotta
            'bg-[hsl(25,60%,45%)] text-white shadow-sm hover:bg-[hsl(25,60%,40%)] focus-visible:ring-[hsl(25,60%,45%)]':
              variant === 'primary',

            // Secondary - muted tan
            'bg-[hsl(45,20%,88%)] text-[hsl(30,10%,20%)] shadow-sm hover:bg-[hsl(45,20%,84%)] focus-visible:ring-[hsl(45,20%,70%)]':
              variant === 'secondary',

            // Ghost - transparent
            'bg-transparent text-[hsl(30,10%,30%)] hover:bg-[hsl(30,10%,90%)] focus-visible:ring-[hsl(30,15%,80%)]':
              variant === 'ghost',

            // Outline - bordered
            'border border-[hsl(30,15%,80%)] bg-transparent text-[hsl(30,10%,25%)] hover:bg-[hsl(35,15%,94%)] focus-visible:ring-[hsl(30,15%,80%)]':
              variant === 'outline',

            // Destructive
            'bg-[hsl(0,60%,50%)] text-white shadow-sm hover:bg-[hsl(0,60%,45%)] focus-visible:ring-[hsl(0,60%,50%)]':
              variant === 'destructive',
          },

          className
        )}
        {...props}
      >
        {children}
      </button>
    );
  }
);

Button.displayName = 'Button';

export { Button };
