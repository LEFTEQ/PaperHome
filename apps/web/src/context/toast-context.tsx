/**
 * Toast Notification Context
 * Uses Radix Toast with glass styling
 */

import {
  createContext,
  useContext,
  useCallback,
  useState,
  useEffect,
  type ReactNode,
} from 'react';
import { setGlobalToastError } from '@/lib/toast-handler';
import * as ToastPrimitive from '@radix-ui/react-toast';
import { motion, AnimatePresence } from 'framer-motion';
import { X, CheckCircle, AlertCircle, AlertTriangle, Info } from 'lucide-react';
import { cn } from '@/lib/utils';

// ─────────────────────────────────────────────────────────────────────────────
// Types
// ─────────────────────────────────────────────────────────────────────────────

export type ToastType = 'success' | 'error' | 'warning' | 'info';

export interface Toast {
  id: string;
  type: ToastType;
  title: string;
  description?: string;
  duration?: number;
}

export interface ToastContextValue {
  /** Show a toast notification */
  toast: (options: Omit<Toast, 'id'>) => void;
  /** Show a success toast */
  success: (title: string, description?: string) => void;
  /** Show an error toast */
  error: (title: string, description?: string) => void;
  /** Show a warning toast */
  warning: (title: string, description?: string) => void;
  /** Show an info toast */
  info: (title: string, description?: string) => void;
  /** Dismiss a toast by id */
  dismiss: (id: string) => void;
  /** Dismiss all toasts */
  dismissAll: () => void;
}

// ─────────────────────────────────────────────────────────────────────────────
// Context
// ─────────────────────────────────────────────────────────────────────────────

const ToastContext = createContext<ToastContextValue | null>(null);

export function useToast(): ToastContextValue {
  const context = useContext(ToastContext);
  if (!context) {
    throw new Error('useToast must be used within a ToastProvider');
  }
  return context;
}

// ─────────────────────────────────────────────────────────────────────────────
// Icons and Styles
// ─────────────────────────────────────────────────────────────────────────────

const toastIcons: Record<ToastType, typeof CheckCircle> = {
  success: CheckCircle,
  error: AlertCircle,
  warning: AlertTriangle,
  info: Info,
};

const toastStyles: Record<ToastType, string> = {
  success: 'border-success/30 bg-success/5',
  error: 'border-error/30 bg-error/5',
  warning: 'border-warning/30 bg-warning/5',
  info: 'border-info/30 bg-info/5',
};

const iconStyles: Record<ToastType, string> = {
  success: 'text-success',
  error: 'text-error',
  warning: 'text-warning',
  info: 'text-info',
};

// ─────────────────────────────────────────────────────────────────────────────
// Toast Component
// ─────────────────────────────────────────────────────────────────────────────

interface ToastItemProps {
  toast: Toast;
  onDismiss: (id: string) => void;
}

function ToastItem({ toast, onDismiss }: ToastItemProps) {
  const Icon = toastIcons[toast.type];

  return (
    <ToastPrimitive.Root
      asChild
      duration={toast.duration || 5000}
      onOpenChange={(open) => {
        if (!open) onDismiss(toast.id);
      }}
    >
      <motion.li
        initial={{ opacity: 0, y: 50, scale: 0.95 }}
        animate={{ opacity: 1, y: 0, scale: 1 }}
        exit={{ opacity: 0, y: 20, scale: 0.95 }}
        transition={{ type: 'spring', stiffness: 500, damping: 30 }}
        className={cn(
          'relative flex items-start gap-3 w-full max-w-sm p-4 rounded-xl',
          'backdrop-blur-xl border shadow-xl',
          'bg-glass border-glass-border',
          toastStyles[toast.type]
        )}
      >
        <Icon className={cn('h-5 w-5 flex-shrink-0 mt-0.5', iconStyles[toast.type])} />

        <div className="flex-1 min-w-0">
          <ToastPrimitive.Title className="text-sm font-semibold text-white">
            {toast.title}
          </ToastPrimitive.Title>
          {toast.description && (
            <ToastPrimitive.Description className="text-sm text-text-muted mt-1">
              {toast.description}
            </ToastPrimitive.Description>
          )}
        </div>

        <ToastPrimitive.Close
          className={cn(
            'flex-shrink-0 h-6 w-6 rounded-md flex items-center justify-center',
            'text-text-muted hover:text-white hover:bg-glass-hover',
            'transition-colors'
          )}
          aria-label="Close"
        >
          <X className="h-4 w-4" />
        </ToastPrimitive.Close>
      </motion.li>
    </ToastPrimitive.Root>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Toast Provider
// ─────────────────────────────────────────────────────────────────────────────

interface ToastProviderProps {
  children: ReactNode;
}

export function ToastProvider({ children }: ToastProviderProps) {
  const [toasts, setToasts] = useState<Toast[]>([]);

  const addToast = useCallback((options: Omit<Toast, 'id'>) => {
    const id = crypto.randomUUID();
    setToasts((prev) => [...prev, { ...options, id }]);
  }, []);

  const dismiss = useCallback((id: string) => {
    setToasts((prev) => prev.filter((t) => t.id !== id));
  }, []);

  const dismissAll = useCallback(() => {
    setToasts([]);
  }, []);

  const toast = useCallback(
    (options: Omit<Toast, 'id'>) => addToast(options),
    [addToast]
  );

  const success = useCallback(
    (title: string, description?: string) =>
      addToast({ type: 'success', title, description }),
    [addToast]
  );

  const error = useCallback(
    (title: string, description?: string) =>
      addToast({ type: 'error', title, description }),
    [addToast]
  );

  const warning = useCallback(
    (title: string, description?: string) =>
      addToast({ type: 'warning', title, description }),
    [addToast]
  );

  const info = useCallback(
    (title: string, description?: string) =>
      addToast({ type: 'info', title, description }),
    [addToast]
  );

  // Register global error handler for React Query
  useEffect(() => {
    setGlobalToastError(error);
    return () => setGlobalToastError(null);
  }, [error]);

  const value: ToastContextValue = {
    toast,
    success,
    error,
    warning,
    info,
    dismiss,
    dismissAll,
  };

  return (
    <ToastContext.Provider value={value}>
      <ToastPrimitive.Provider swipeDirection="right">
        {children}

        <ToastPrimitive.Viewport asChild>
          <ul
            className={cn(
              'fixed bottom-0 right-0 z-[100] flex flex-col gap-2 p-4',
              'max-h-screen w-full sm:w-auto',
              'outline-none'
            )}
          >
            <AnimatePresence mode="popLayout">
              {toasts.map((t) => (
                <ToastItem key={t.id} toast={t} onDismiss={dismiss} />
              ))}
            </AnimatePresence>
          </ul>
        </ToastPrimitive.Viewport>
      </ToastPrimitive.Provider>
    </ToastContext.Provider>
  );
}

export default ToastProvider;
