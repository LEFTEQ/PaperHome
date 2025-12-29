import { forwardRef, InputHTMLAttributes, TextareaHTMLAttributes, useState } from 'react';
import { cva, type VariantProps } from 'class-variance-authority';
import { Eye, EyeOff, Search, AlertCircle } from 'lucide-react';
import { cn } from '@/lib/utils';

const inputVariants = cva(
  [
    'flex w-full rounded-xl border',
    'bg-white/[0.03] backdrop-blur-sm',
    'border-white/[0.08]',
    'text-white placeholder:text-[hsl(228,10%,40%)]',
    'transition-all duration-200',
    'focus:outline-none focus:border-[hsl(187,100%,50%)]',
    'focus:ring-2 focus:ring-[hsl(187,100%,50%,0.15)]',
    'disabled:cursor-not-allowed disabled:opacity-50',
    'hover:border-white/[0.12]',
  ],
  {
    variants: {
      inputSize: {
        sm: 'h-9 px-3 text-sm',
        md: 'h-11 px-4 text-sm',
        lg: 'h-12 px-4 text-base',
      },
      hasError: {
        true: [
          'border-[hsl(0,72%,51%)]',
          'focus:border-[hsl(0,72%,51%)]',
          'focus:ring-[hsl(0,72%,51%,0.15)]',
        ],
        false: '',
      },
      hasIcon: {
        left: 'pl-10',
        right: 'pr-10',
        both: 'pl-10 pr-10',
        none: '',
      },
    },
    defaultVariants: {
      inputSize: 'md',
      hasError: false,
      hasIcon: 'none',
    },
  }
);

export interface InputProps
  extends Omit<InputHTMLAttributes<HTMLInputElement>, 'size'>,
    Omit<VariantProps<typeof inputVariants>, 'hasError' | 'hasIcon'> {
  /** Label text displayed above the input */
  label?: string;
  /** Error message displayed below the input */
  error?: string;
  /** Helper text displayed below the input */
  helperText?: string;
  /** Icon displayed on the left side */
  leftIcon?: React.ReactNode;
  /** Icon displayed on the right side */
  rightIcon?: React.ReactNode;
  /** Show character count for maxLength */
  showCount?: boolean;
}

const Input = forwardRef<HTMLInputElement, InputProps>(
  (
    {
      className,
      type = 'text',
      inputSize,
      label,
      error,
      helperText,
      leftIcon,
      rightIcon,
      showCount,
      maxLength,
      id,
      value,
      onChange,
      ...props
    },
    ref
  ) => {
    const inputId = id || label?.toLowerCase().replace(/\s+/g, '-');
    const [charCount, setCharCount] = useState((value as string)?.length || 0);

    const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
      if (showCount) {
        setCharCount(e.target.value.length);
      }
      onChange?.(e);
    };

    const hasIcon = leftIcon && rightIcon ? 'both' : leftIcon ? 'left' : rightIcon ? 'right' : 'none';

    return (
      <div className="space-y-2">
        {label && (
          <label
            htmlFor={inputId}
            className="block text-sm font-medium text-[hsl(228,10%,70%)]"
          >
            {label}
          </label>
        )}
        <div className="relative">
          {leftIcon && (
            <div className="absolute left-3 top-1/2 -translate-y-1/2 text-[hsl(228,10%,40%)]">
              {leftIcon}
            </div>
          )}
          <input
            type={type}
            id={inputId}
            ref={ref}
            value={value}
            onChange={handleChange}
            maxLength={maxLength}
            className={cn(
              inputVariants({
                inputSize,
                hasError: !!error,
                hasIcon,
              }),
              className
            )}
            {...props}
          />
          {rightIcon && (
            <div className="absolute right-3 top-1/2 -translate-y-1/2 text-[hsl(228,10%,40%)]">
              {rightIcon}
            </div>
          )}
          {error && !rightIcon && (
            <div className="absolute right-3 top-1/2 -translate-y-1/2 text-[hsl(0,72%,51%)]">
              <AlertCircle className="h-4 w-4" />
            </div>
          )}
        </div>
        <div className="flex justify-between">
          {(error || helperText) && (
            <p
              className={cn(
                'text-xs',
                error ? 'text-[hsl(0,72%,51%)]' : 'text-[hsl(228,10%,50%)]'
              )}
            >
              {error || helperText}
            </p>
          )}
          {showCount && maxLength && (
            <p className="text-xs text-[hsl(228,10%,50%)] ml-auto">
              {charCount}/{maxLength}
            </p>
          )}
        </div>
      </div>
    );
  }
);

Input.displayName = 'Input';

// ─────────────────────────────────────────────────────────────────────────────
// Password Input with toggle visibility
// ─────────────────────────────────────────────────────────────────────────────

export interface PasswordInputProps extends Omit<InputProps, 'type' | 'rightIcon'> {}

const PasswordInput = forwardRef<HTMLInputElement, PasswordInputProps>(
  ({ ...props }, ref) => {
    const [showPassword, setShowPassword] = useState(false);

    return (
      <Input
        ref={ref}
        type={showPassword ? 'text' : 'password'}
        rightIcon={
          <button
            type="button"
            onClick={() => setShowPassword(!showPassword)}
            className="text-[hsl(228,10%,40%)] hover:text-white transition-colors"
            tabIndex={-1}
          >
            {showPassword ? (
              <EyeOff className="h-4 w-4" />
            ) : (
              <Eye className="h-4 w-4" />
            )}
          </button>
        }
        {...props}
      />
    );
  }
);

PasswordInput.displayName = 'PasswordInput';

// ─────────────────────────────────────────────────────────────────────────────
// Search Input with search icon
// ─────────────────────────────────────────────────────────────────────────────

export interface SearchInputProps extends Omit<InputProps, 'leftIcon'> {
  onSearch?: (value: string) => void;
}

const SearchInput = forwardRef<HTMLInputElement, SearchInputProps>(
  ({ onSearch, onKeyDown, ...props }, ref) => {
    const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
      if (e.key === 'Enter' && onSearch) {
        onSearch((e.target as HTMLInputElement).value);
      }
      onKeyDown?.(e);
    };

    return (
      <Input
        ref={ref}
        type="search"
        leftIcon={<Search className="h-4 w-4" />}
        onKeyDown={handleKeyDown}
        {...props}
      />
    );
  }
);

SearchInput.displayName = 'SearchInput';

// ─────────────────────────────────────────────────────────────────────────────
// Textarea
// ─────────────────────────────────────────────────────────────────────────────

export interface TextareaProps
  extends TextareaHTMLAttributes<HTMLTextAreaElement> {
  label?: string;
  error?: string;
  helperText?: string;
  showCount?: boolean;
}

const Textarea = forwardRef<HTMLTextAreaElement, TextareaProps>(
  (
    {
      className,
      label,
      error,
      helperText,
      showCount,
      maxLength,
      id,
      value,
      onChange,
      ...props
    },
    ref
  ) => {
    const inputId = id || label?.toLowerCase().replace(/\s+/g, '-');
    const [charCount, setCharCount] = useState((value as string)?.length || 0);

    const handleChange = (e: React.ChangeEvent<HTMLTextAreaElement>) => {
      if (showCount) {
        setCharCount(e.target.value.length);
      }
      onChange?.(e);
    };

    return (
      <div className="space-y-2">
        {label && (
          <label
            htmlFor={inputId}
            className="block text-sm font-medium text-[hsl(228,10%,70%)]"
          >
            {label}
          </label>
        )}
        <textarea
          id={inputId}
          ref={ref}
          value={value}
          onChange={handleChange}
          maxLength={maxLength}
          className={cn(
            'flex w-full rounded-xl border min-h-[100px] p-4',
            'bg-white/[0.03] backdrop-blur-sm',
            'border-white/[0.08]',
            'text-white placeholder:text-[hsl(228,10%,40%)]',
            'transition-all duration-200',
            'focus:outline-none focus:border-[hsl(187,100%,50%)]',
            'focus:ring-2 focus:ring-[hsl(187,100%,50%,0.15)]',
            'disabled:cursor-not-allowed disabled:opacity-50',
            'hover:border-white/[0.12]',
            'resize-none',
            error && [
              'border-[hsl(0,72%,51%)]',
              'focus:border-[hsl(0,72%,51%)]',
              'focus:ring-[hsl(0,72%,51%,0.15)]',
            ],
            className
          )}
          {...props}
        />
        <div className="flex justify-between">
          {(error || helperText) && (
            <p
              className={cn(
                'text-xs',
                error ? 'text-[hsl(0,72%,51%)]' : 'text-[hsl(228,10%,50%)]'
              )}
            >
              {error || helperText}
            </p>
          )}
          {showCount && maxLength && (
            <p className="text-xs text-[hsl(228,10%,50%)] ml-auto">
              {charCount}/{maxLength}
            </p>
          )}
        </div>
      </div>
    );
  }
);

Textarea.displayName = 'Textarea';

export { Input, PasswordInput, SearchInput, Textarea, inputVariants };
