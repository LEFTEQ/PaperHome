import { ReactNode } from 'react';
import { motion } from 'framer-motion';
import { LucideIcon, Inbox, Search, Wifi, AlertCircle } from 'lucide-react';
import { Button } from './button';
import { GlassCard } from './glass-card';
import { cn } from '@/lib/utils';
import { fadeInUp } from '@/lib/animations';

export interface EmptyStateProps {
  /** Icon to display */
  icon?: LucideIcon;
  /** Custom icon element */
  iconElement?: ReactNode;
  /** Title text */
  title: string;
  /** Description text */
  description?: string;
  /** Primary action */
  action?: {
    label: string;
    onClick: () => void;
    variant?: 'primary' | 'secondary' | 'glassAccent';
  };
  /** Secondary action */
  secondaryAction?: {
    label: string;
    onClick: () => void;
  };
  /** Size variant */
  size?: 'sm' | 'md' | 'lg';
  /** Show in a glass card */
  withCard?: boolean;
  /** Additional className */
  className?: string;
}

export function EmptyState({
  icon: Icon = Inbox,
  iconElement,
  title,
  description,
  action,
  secondaryAction,
  size = 'md',
  withCard = true,
  className,
}: EmptyStateProps) {
  const sizeClasses = {
    sm: {
      icon: 'h-10 w-10',
      iconWrapper: 'h-16 w-16',
      title: 'text-base',
      description: 'text-sm',
      padding: 'p-6',
    },
    md: {
      icon: 'h-12 w-12',
      iconWrapper: 'h-20 w-20',
      title: 'text-lg',
      description: 'text-sm',
      padding: 'p-8',
    },
    lg: {
      icon: 'h-16 w-16',
      iconWrapper: 'h-24 w-24',
      title: 'text-xl',
      description: 'text-base',
      padding: 'p-12',
    },
  };

  const sizes = sizeClasses[size];

  const content = (
    <motion.div
      className={cn(
        'flex flex-col items-center text-center',
        !withCard && sizes.padding,
        className
      )}
      variants={fadeInUp}
      initial="initial"
      animate="animate"
    >
      {/* Icon */}
      <div
        className={cn(
          'flex items-center justify-center rounded-2xl mb-4',
          sizes.iconWrapper,
          'bg-glass border border-glass-border'
        )}
      >
        {iconElement || (
          <Icon className={cn(sizes.icon, 'text-text-subtle')} />
        )}
      </div>

      {/* Title */}
      <h3 className={cn('font-semibold text-white mb-2', sizes.title)}>
        {title}
      </h3>

      {/* Description */}
      {description && (
        <p
          className={cn(
            'text-text-muted max-w-sm mb-6',
            sizes.description
          )}
        >
          {description}
        </p>
      )}

      {/* Actions */}
      {(action || secondaryAction) && (
        <div className="flex items-center gap-3">
          {action && (
            <Button
              variant={action.variant || 'primary'}
              onClick={action.onClick}
            >
              {action.label}
            </Button>
          )}
          {secondaryAction && (
            <Button variant="ghost" onClick={secondaryAction.onClick}>
              {secondaryAction.label}
            </Button>
          )}
        </div>
      )}
    </motion.div>
  );

  if (withCard) {
    return (
      <GlassCard className={cn(sizes.padding, className)}>
        {content}
      </GlassCard>
    );
  }

  return content;
}

// ─────────────────────────────────────────────────────────────────────────────
// Preset empty states
// ─────────────────────────────────────────────────────────────────────────────

export interface PresetEmptyStateProps {
  action?: EmptyStateProps['action'];
  className?: string;
}

export function NoDevicesState({ action, className }: PresetEmptyStateProps) {
  return (
    <EmptyState
      icon={Wifi}
      title="No devices yet"
      description="Connect your first ESP32 device to start monitoring your home environment."
      action={
        action || {
          label: 'Add Device',
          onClick: () => {},
        }
      }
      className={className}
    />
  );
}

export function NoDataState({ action, className }: PresetEmptyStateProps) {
  return (
    <EmptyState
      icon={Inbox}
      title="No data available"
      description="This device hasn't reported any data yet. Make sure it's connected and configured correctly."
      action={action}
      className={className}
    />
  );
}

export function NoResultsState({ action, className }: PresetEmptyStateProps) {
  return (
    <EmptyState
      icon={Search}
      title="No results found"
      description="Try adjusting your search or filters to find what you're looking for."
      action={action}
      size="sm"
      className={className}
    />
  );
}

export function ErrorState({
  action,
  className,
  message,
}: PresetEmptyStateProps & { message?: string }) {
  return (
    <EmptyState
      icon={AlertCircle}
      title="Something went wrong"
      description={message || "We couldn't load the data. Please try again."}
      action={
        action || {
          label: 'Try Again',
          onClick: () => window.location.reload(),
          variant: 'secondary',
        }
      }
      className={className}
    />
  );
}

export function OfflineState({ className }: { className?: string }) {
  return (
    <EmptyState
      icon={Wifi}
      title="Device offline"
      description="This device is currently not connected. Check the device's power and network connection."
      size="sm"
      className={className}
    />
  );
}

export default EmptyState;
