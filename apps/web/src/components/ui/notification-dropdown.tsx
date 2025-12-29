import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Bell, Check, X, AlertTriangle, Info, Wifi, WifiOff } from 'lucide-react';
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuTrigger,
  DropdownMenuSeparator,
} from './dropdown';
import { cn } from '@/lib/utils';
import { fadeInUp } from '@/lib/animations';

export interface Notification {
  id: string;
  type: 'info' | 'warning' | 'error' | 'success' | 'device_online' | 'device_offline';
  title: string;
  message: string;
  timestamp: Date;
  read: boolean;
  deviceId?: string;
}

export interface NotificationDropdownProps {
  notifications: Notification[];
  onMarkAsRead: (id: string) => void;
  onMarkAllAsRead: () => void;
  onDismiss: (id: string) => void;
  onClearAll: () => void;
}

const notificationIcons = {
  info: Info,
  warning: AlertTriangle,
  error: AlertTriangle,
  success: Check,
  device_online: Wifi,
  device_offline: WifiOff,
};

const notificationColors = {
  info: {
    icon: 'text-info',
    bg: 'bg-info/10',
    border: 'border-info/30',
  },
  warning: {
    icon: 'text-warning',
    bg: 'bg-warning-bg',
    border: 'border-warning/30',
  },
  error: {
    icon: 'text-error',
    bg: 'bg-error-bg',
    border: 'border-error/30',
  },
  success: {
    icon: 'text-success',
    bg: 'bg-success-bg',
    border: 'border-success/30',
  },
  device_online: {
    icon: 'text-online',
    bg: 'bg-success-bg',
    border: 'border-success/30',
  },
  device_offline: {
    icon: 'text-text-muted',
    bg: 'bg-glass-hover',
    border: 'border-glass-border-hover',
  },
};

function formatTimeAgo(date: Date): string {
  const now = new Date();
  const diffMs = now.getTime() - date.getTime();
  const diffMins = Math.floor(diffMs / 60000);
  const diffHours = Math.floor(diffMs / 3600000);
  const diffDays = Math.floor(diffMs / 86400000);

  if (diffMins < 1) return 'Just now';
  if (diffMins < 60) return `${diffMins}m ago`;
  if (diffHours < 24) return `${diffHours}h ago`;
  if (diffDays < 7) return `${diffDays}d ago`;
  return date.toLocaleDateString();
}

export function NotificationDropdown({
  notifications,
  onMarkAsRead,
  onMarkAllAsRead,
  onDismiss,
  onClearAll,
}: NotificationDropdownProps) {
  const [isOpen, setIsOpen] = useState(false);
  const unreadCount = notifications.filter((n) => !n.read).length;

  return (
    <DropdownMenu open={isOpen} onOpenChange={setIsOpen}>
      <DropdownMenuTrigger asChild>
        <button
          className={cn(
            'relative p-2 rounded-xl',
            'text-text-muted hover:text-white',
            'hover:bg-glass-hover',
            'transition-colors duration-200',
            'focus:outline-none focus:ring-2 focus:ring-accent/50'
          )}
        >
          <Bell className="h-5 w-5" />
          {unreadCount > 0 && (
            <span
              className={cn(
                'absolute -top-0.5 -right-0.5 h-4 min-w-4 px-1',
                'flex items-center justify-center',
                'rounded-full text-[10px] font-bold',
                'bg-error text-white',
                'shadow-[0_0_8px_rgb(239_68_68/0.5)]'
              )}
            >
              {unreadCount > 9 ? '9+' : unreadCount}
            </span>
          )}
        </button>
      </DropdownMenuTrigger>

      <DropdownMenuContent
        align="end"
        className="w-80 p-0"
        sideOffset={12}
      >
        {/* Header */}
        <div className="flex items-center justify-between px-4 py-3 border-b border-glass-border">
          <h3 className="text-sm font-semibold text-white">Notifications</h3>
          {notifications.length > 0 && (
            <div className="flex items-center gap-2">
              {unreadCount > 0 && (
                <button
                  onClick={onMarkAllAsRead}
                  className="text-xs text-accent hover:underline"
                >
                  Mark all read
                </button>
              )}
              <button
                onClick={onClearAll}
                className="text-xs text-text-muted hover:text-white"
              >
                Clear all
              </button>
            </div>
          )}
        </div>

        {/* Notification list */}
        <div className="max-h-[400px] overflow-y-auto">
          {notifications.length === 0 ? (
            <div className="py-12 text-center">
              <Bell className="h-10 w-10 mx-auto text-text-subtle mb-3" />
              <p className="text-sm text-text-muted">No notifications</p>
            </div>
          ) : (
            <AnimatePresence mode="popLayout">
              {notifications.map((notification) => {
                const Icon = notificationIcons[notification.type];
                const colors = notificationColors[notification.type];

                return (
                  <motion.div
                    key={notification.id}
                    layout
                    variants={fadeInUp}
                    initial="initial"
                    animate="animate"
                    exit="exit"
                    className={cn(
                      'group relative px-4 py-3 border-b border-glass-border',
                      'hover:bg-glass',
                      'transition-colors duration-200',
                      !notification.read && 'bg-glass'
                    )}
                  >
                    <div className="flex gap-3">
                      {/* Icon */}
                      <div
                        className={cn(
                          'flex-shrink-0 h-8 w-8 rounded-lg',
                          'flex items-center justify-center',
                          colors.bg,
                          'border',
                          colors.border
                        )}
                      >
                        <Icon className={cn('h-4 w-4', colors.icon)} />
                      </div>

                      {/* Content */}
                      <div className="flex-1 min-w-0">
                        <div className="flex items-start justify-between gap-2">
                          <p
                            className={cn(
                              'text-sm font-medium truncate',
                              notification.read
                                ? 'text-text-secondary'
                                : 'text-white'
                            )}
                          >
                            {notification.title}
                          </p>
                          {!notification.read && (
                            <span className="flex-shrink-0 h-2 w-2 rounded-full bg-accent" />
                          )}
                        </div>
                        <p className="text-xs text-text-muted mt-0.5 line-clamp-2">
                          {notification.message}
                        </p>
                        <p className="text-[10px] text-text-subtle mt-1">
                          {formatTimeAgo(notification.timestamp)}
                        </p>
                      </div>

                      {/* Actions */}
                      <div className="flex-shrink-0 opacity-0 group-hover:opacity-100 transition-opacity">
                        <button
                          onClick={(e) => {
                            e.stopPropagation();
                            onDismiss(notification.id);
                          }}
                          className={cn(
                            'p-1 rounded-md',
                            'text-text-subtle hover:text-white',
                            'hover:bg-glass-hover'
                          )}
                        >
                          <X className="h-3.5 w-3.5" />
                        </button>
                      </div>
                    </div>

                    {/* Click to mark as read */}
                    {!notification.read && (
                      <button
                        onClick={() => onMarkAsRead(notification.id)}
                        className="absolute inset-0"
                        aria-label="Mark as read"
                      />
                    )}
                  </motion.div>
                );
              })}
            </AnimatePresence>
          )}
        </div>
      </DropdownMenuContent>
    </DropdownMenu>
  );
}

export default NotificationDropdown;
