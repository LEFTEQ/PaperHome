import { forwardRef, InputHTMLAttributes } from 'react';
import { cn } from '@/lib/utils';

export interface InputProps extends InputHTMLAttributes<HTMLInputElement> {
  label?: string;
  error?: string;
}

const Input = forwardRef<HTMLInputElement, InputProps>(
  ({ className, type, label, error, id, ...props }, ref) => {
    const inputId = id || label?.toLowerCase().replace(/\s+/g, '-');

    return (
      <div className="space-y-1.5">
        {label && (
          <label
            htmlFor={inputId}
            className="block text-sm font-medium text-[hsl(30,10%,25%)]"
          >
            {label}
          </label>
        )}
        <input
          type={type}
          id={inputId}
          ref={ref}
          className={cn(
            'flex h-10 w-full rounded-lg border bg-[hsl(40,20%,98%)] px-3 py-2 text-sm',
            'border-[hsl(30,15%,80%)]',
            'placeholder:text-[hsl(30,10%,60%)]',
            'transition-all duration-200',
            'focus:outline-none focus:ring-2 focus:ring-[hsl(25,60%,45%)] focus:ring-offset-1 focus:border-[hsl(25,60%,45%)]',
            'disabled:cursor-not-allowed disabled:opacity-50 disabled:bg-[hsl(30,10%,92%)]',
            error && 'border-[hsl(0,60%,50%)] focus:ring-[hsl(0,60%,50%)]',
            className
          )}
          {...props}
        />
        {error && (
          <p className="text-xs text-[hsl(0,60%,50%)] mt-1">{error}</p>
        )}
      </div>
    );
  }
);

Input.displayName = 'Input';

export { Input };
