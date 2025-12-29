import { Fragment, forwardRef, HTMLAttributes } from 'react';
import * as DialogPrimitive from '@radix-ui/react-dialog';
import { motion, AnimatePresence } from 'framer-motion';
import { X } from 'lucide-react';
import { cn } from '@/lib/utils';
import { overlayAnimation, modalAnimation } from '@/lib/animations';

const Modal = DialogPrimitive.Root;
const ModalTrigger = DialogPrimitive.Trigger;
const ModalPortal = DialogPrimitive.Portal;
const ModalClose = DialogPrimitive.Close;

const ModalOverlay = forwardRef<
  React.ElementRef<typeof DialogPrimitive.Overlay>,
  React.ComponentPropsWithoutRef<typeof DialogPrimitive.Overlay>
>(({ className, ...props }, ref) => (
  <DialogPrimitive.Overlay
    ref={ref}
    className={cn(
      'fixed inset-0 z-50 bg-black/60 backdrop-blur-sm',
      className
    )}
    {...props}
  />
));
ModalOverlay.displayName = 'ModalOverlay';

export interface ModalContentProps
  extends React.ComponentPropsWithoutRef<typeof DialogPrimitive.Content> {
  /** Show close button */
  showClose?: boolean;
  /** Size variant */
  size?: 'sm' | 'md' | 'lg' | 'xl' | 'full';
}

const ModalContent = forwardRef<
  React.ElementRef<typeof DialogPrimitive.Content>,
  ModalContentProps
>(({ className, children, showClose = true, size = 'md', ...props }, ref) => {
  const sizeClasses = {
    sm: 'max-w-sm',
    md: 'max-w-lg',
    lg: 'max-w-2xl',
    xl: 'max-w-4xl',
    full: 'max-w-[calc(100vw-2rem)] max-h-[calc(100vh-2rem)]',
  };

  return (
    <ModalPortal>
      <AnimatePresence>
        <ModalOverlay asChild>
          <motion.div {...overlayAnimation} />
        </ModalOverlay>
        <DialogPrimitive.Content
          ref={ref}
          className={cn(
            'fixed left-[50%] top-[50%] z-50 w-full translate-x-[-50%] translate-y-[-50%]',
            sizeClasses[size],
            className
          )}
          {...props}
        >
          <motion.div
            className={cn(
              'rounded-2xl border border-white/[0.08]',
              'bg-[hsl(228,12%,8%)]/95 backdrop-blur-xl',
              'shadow-2xl',
              'p-6',
              'focus:outline-none'
            )}
            {...modalAnimation}
          >
            {showClose && (
              <DialogPrimitive.Close
                className={cn(
                  'absolute right-4 top-4 rounded-lg p-1.5',
                  'text-[hsl(228,10%,50%)] hover:text-white',
                  'hover:bg-white/[0.05]',
                  'transition-colors duration-200',
                  'focus:outline-none focus:ring-2 focus:ring-[hsl(187,100%,50%)]'
                )}
              >
                <X className="h-4 w-4" />
                <span className="sr-only">Close</span>
              </DialogPrimitive.Close>
            )}
            {children}
          </motion.div>
        </DialogPrimitive.Content>
      </AnimatePresence>
    </ModalPortal>
  );
});
ModalContent.displayName = 'ModalContent';

const ModalHeader = forwardRef<HTMLDivElement, HTMLAttributes<HTMLDivElement>>(
  ({ className, ...props }, ref) => (
    <div
      ref={ref}
      className={cn('flex flex-col gap-1.5 mb-4', className)}
      {...props}
    />
  )
);
ModalHeader.displayName = 'ModalHeader';

const ModalTitle = forwardRef<
  React.ElementRef<typeof DialogPrimitive.Title>,
  React.ComponentPropsWithoutRef<typeof DialogPrimitive.Title>
>(({ className, ...props }, ref) => (
  <DialogPrimitive.Title
    ref={ref}
    className={cn('text-lg font-semibold text-white', className)}
    {...props}
  />
));
ModalTitle.displayName = 'ModalTitle';

const ModalDescription = forwardRef<
  React.ElementRef<typeof DialogPrimitive.Description>,
  React.ComponentPropsWithoutRef<typeof DialogPrimitive.Description>
>(({ className, ...props }, ref) => (
  <DialogPrimitive.Description
    ref={ref}
    className={cn('text-sm text-[hsl(228,10%,60%)]', className)}
    {...props}
  />
));
ModalDescription.displayName = 'ModalDescription';

const ModalBody = forwardRef<HTMLDivElement, HTMLAttributes<HTMLDivElement>>(
  ({ className, ...props }, ref) => (
    <div ref={ref} className={cn('', className)} {...props} />
  )
);
ModalBody.displayName = 'ModalBody';

const ModalFooter = forwardRef<HTMLDivElement, HTMLAttributes<HTMLDivElement>>(
  ({ className, ...props }, ref) => (
    <div
      ref={ref}
      className={cn(
        'flex items-center justify-end gap-3 mt-6 pt-4',
        'border-t border-white/[0.06]',
        className
      )}
      {...props}
    />
  )
);
ModalFooter.displayName = 'ModalFooter';

// ─────────────────────────────────────────────────────────────────────────────
// Confirmation Modal - Common pattern for destructive actions
// ─────────────────────────────────────────────────────────────────────────────

export interface ConfirmModalProps {
  open: boolean;
  onOpenChange: (open: boolean) => void;
  title: string;
  description?: string;
  confirmLabel?: string;
  cancelLabel?: string;
  onConfirm: () => void;
  onCancel?: () => void;
  variant?: 'default' | 'destructive';
  isLoading?: boolean;
}

function ConfirmModal({
  open,
  onOpenChange,
  title,
  description,
  confirmLabel = 'Confirm',
  cancelLabel = 'Cancel',
  onConfirm,
  onCancel,
  variant = 'default',
  isLoading = false,
}: ConfirmModalProps) {
  const handleCancel = () => {
    onCancel?.();
    onOpenChange(false);
  };

  const handleConfirm = () => {
    onConfirm();
    if (!isLoading) {
      onOpenChange(false);
    }
  };

  return (
    <Modal open={open} onOpenChange={onOpenChange}>
      <ModalContent size="sm" showClose={false}>
        <ModalHeader>
          <ModalTitle>{title}</ModalTitle>
          {description && <ModalDescription>{description}</ModalDescription>}
        </ModalHeader>
        <ModalFooter className="border-t-0 mt-4 pt-0">
          <button
            onClick={handleCancel}
            disabled={isLoading}
            className={cn(
              'px-4 py-2 rounded-lg text-sm font-medium',
              'text-[hsl(228,10%,70%)] hover:text-white',
              'hover:bg-white/[0.05]',
              'transition-colors duration-200',
              'disabled:opacity-50'
            )}
          >
            {cancelLabel}
          </button>
          <button
            onClick={handleConfirm}
            disabled={isLoading}
            className={cn(
              'px-4 py-2 rounded-lg text-sm font-semibold',
              'transition-all duration-200',
              'disabled:opacity-50',
              variant === 'destructive'
                ? [
                    'bg-[hsl(0,72%,51%)] text-white',
                    'hover:bg-[hsl(0,72%,45%)]',
                    'shadow-[0_0_15px_hsla(0,72%,51%,0.3)]',
                  ]
                : [
                    'bg-[hsl(187,100%,50%)] text-[hsl(228,15%,4%)]',
                    'hover:bg-[hsl(187,100%,55%)]',
                    'shadow-[0_0_15px_hsla(187,100%,50%,0.3)]',
                  ]
            )}
          >
            {isLoading ? 'Loading...' : confirmLabel}
          </button>
        </ModalFooter>
      </ModalContent>
    </Modal>
  );
}

export {
  Modal,
  ModalTrigger,
  ModalPortal,
  ModalOverlay,
  ModalContent,
  ModalHeader,
  ModalTitle,
  ModalDescription,
  ModalBody,
  ModalFooter,
  ModalClose,
  ConfirmModal,
};
