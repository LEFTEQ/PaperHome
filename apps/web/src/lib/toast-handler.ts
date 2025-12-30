/**
 * Global toast error handler for React Query
 * This is used to show toasts from outside of React components
 */

type ToastErrorFn = (title: string, description?: string) => void;

let globalToastError: ToastErrorFn | null = null;

export function setGlobalToastError(fn: ToastErrorFn | null) {
  globalToastError = fn;
}

export function showGlobalError(title: string, description?: string) {
  globalToastError?.(title, description);
}
