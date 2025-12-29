import { HTMLAttributes } from 'react';
import { cn } from '@/lib/utils';

export interface BadgeProps extends HTMLAttributes<HTMLSpanElement> {
  variant?: 'default' | 'success' | 'warning' | 'error' | 'outline';
}

function Badge({ className, variant = 'default', ...props }: BadgeProps) {
  return (
    <span
      className={cn(
        'inline-flex items-center rounded-full px-2.5 py-0.5 text-xs font-medium',
        'transition-colors duration-150',
        {
          // Default - muted background
          'bg-[hsl(30,10%,88%)] text-[hsl(30,10%,30%)]': variant === 'default',

          // Success - sage green
          'bg-[hsl(140,25%,90%)] text-[hsl(140,30%,30%)]': variant === 'success',

          // Warning - warm amber
          'bg-[hsl(40,60%,90%)] text-[hsl(35,60%,30%)]': variant === 'warning',

          // Error - muted red
          'bg-[hsl(0,40%,92%)] text-[hsl(0,50%,40%)]': variant === 'error',

          // Outline
          'border border-[hsl(30,15%,80%)] bg-transparent text-[hsl(30,10%,35%)]':
            variant === 'outline',
        },
        className
      )}
      {...props}
    />
  );
}

export { Badge };
