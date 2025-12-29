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
    icon: 'text-[hsl(199,89%,48%)]',
    bg: 'bg-[hsl(199,89%,48%,0.1)]',
    border: 'border-[hsl(199,89%,48%,0.3)]',
  },
  warning: {
    icon: 'text-[hsl(38,92%,50%)]',
    bg: 'bg-[hsl(38,92%,50%,0.1)]',
    border: 'border-[hsl(38,92%,50%,0.3)]',
  },
  error: {
    icon: 'text-[hsl(0,72%,51%)]',
    bg: 'bg-[hsl(0,72%,51%,0.1)]',
    border: 'border-[hsl(0,72%,51%,0.3)]',
  },
  success: {
    icon: 'text-[hsl(160,84%,39%)]',
    bg: 'bg-[hsl(160,84%,39%,0.1)]',
    border: 'border-[hsl(160,84%,39%,0.3)]',
  },
  device_online: {
    icon: 'text-[hsl(160,84%,39%)]',
    bg: 'bg-[hsl(160,84%,39%,0.1)]',
    border: 'border-[hsl(160,84%,39%,0.3)]',
  },
  device_offline: {
    icon: 'text-[hsl(228,10%,50%)]',
    bg: 'bg-white/[0.05]',
    border: 'border-white/[0.1]',
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
            'text-[hsl(228,10%,60%)] hover:text-white',
            'hover:bg-white/[0.05]',
            'transition-colors duration-200',
            'focus:outline-none focus:ring-2 focus:ring-[hsl(187,100%,50%,0.5)]'
          )}
        >
          <Bell className="h-5 w-5" />
          {unreadCount > 0 && (
            <span
              className={cn(
                'absolute -top-0.5 -right-0.5 h-4 min-w-4 px-1',
                'flex items-center justify-center',
                'rounded-full text-[10px] font-bold',
                'bg-[hsl(0,72%,51%)] text-white',
                'shadow-[0_0_8px_hsla(0,72%,51%,0.5)]'
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
        <div className="flex items-center justify-between px-4 py-3 border-b border-white/[0.06]">
          <h3 className="text-sm font-semibold text-white">Notifications</h3>
          {notifications.length > 0 && (
            <div className="flex items-center gap-2">
              {unreadCount > 0 && (
                <button
                  onClick={onMarkAllAsRead}
                  className="text-xs text-[hsl(187,100%,50%)] hover:underline"
                >
                  Mark all read
                </button>
              )}
              <button
                onClick={onClearAll}
                className="text-xs text-[hsl(228,10%,50%)] hover:text-white"
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
              <Bell className="h-10 w-10 mx-auto text-[hsl(228,10%,30%)] mb-3" />
              <p className="text-sm text-[hsl(228,10%,50%)]">No notifications</p>
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
                      'group relative px-4 py-3 border-b border-white/[0.04]',
                      'hover:bg-white/[0.02]',
                      'transition-colors duration-200',
                      !notification.read && 'bg-white/[0.02]'
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
                                ? 'text-[hsl(228,10%,70%)]'
                                : 'text-white'
                            )}
                          >
                            {notification.title}
                          </p>
                          {!notification.read && (
                            <span className="flex-shrink-0 h-2 w-2 rounded-full bg-[hsl(187,100%,50%)]" />
                          )}
                        </div>
                        <p className="text-xs text-[hsl(228,10%,50%)] mt-0.5 line-clamp-2">
                          {notification.message}
                        </p>
                        <p className="text-[10px] text-[hsl(228,10%,40%)] mt-1">
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
                            'text-[hsl(228,10%,40%)] hover:text-white',
                            'hover:bg-white/[0.05]'
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
