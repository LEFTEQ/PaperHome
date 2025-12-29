import { useState, useCallback, useEffect, createContext, useContext, type ReactNode } from 'react';
import { useWebSocketMessage } from './use-websocket';
import type { NotificationPayload } from '@/lib/websocket';

// ─────────────────────────────────────────────────────────────────────────────
// Types
// ─────────────────────────────────────────────────────────────────────────────

export interface Notification extends NotificationPayload {
  // Extended with client-side properties
}

export interface UseNotificationsReturn {
  /** All notifications */
  notifications: Notification[];
  /** Unread notifications count */
  unreadCount: number;
  /** Mark a notification as read */
  markAsRead: (id: string) => void;
  /** Mark all notifications as read */
  markAllAsRead: () => void;
  /** Remove a notification */
  remove: (id: string) => void;
  /** Clear all notifications */
  clearAll: () => void;
  /** Add a local notification (for testing/UI) */
  addNotification: (notification: Omit<Notification, 'id' | 'createdAt'>) => void;
}

// ─────────────────────────────────────────────────────────────────────────────
// Storage
// ─────────────────────────────────────────────────────────────────────────────

const STORAGE_KEY = 'paperhome_notifications';
const MAX_NOTIFICATIONS = 50;

function loadNotifications(): Notification[] {
  try {
    const stored = localStorage.getItem(STORAGE_KEY);
    if (!stored) return [];
    return JSON.parse(stored);
  } catch {
    return [];
  }
}

function saveNotifications(notifications: Notification[]): void {
  try {
    // Keep only recent notifications
    const toSave = notifications.slice(0, MAX_NOTIFICATIONS);
    localStorage.setItem(STORAGE_KEY, JSON.stringify(toSave));
  } catch {
    // Ignore storage errors
  }
}

// ─────────────────────────────────────────────────────────────────────────────
// useNotifications
// ─────────────────────────────────────────────────────────────────────────────

export function useNotifications(): UseNotificationsReturn {
  const [notifications, setNotifications] = useState<Notification[]>(() =>
    loadNotifications()
  );

  // Persist to localStorage when notifications change
  useEffect(() => {
    saveNotifications(notifications);
  }, [notifications]);

  // Listen for WebSocket notifications
  useWebSocketMessage<NotificationPayload>('notification', (message) => {
    const notification = message.payload;

    setNotifications((prev) => {
      // Avoid duplicates
      if (prev.some((n) => n.id === notification.id)) {
        return prev;
      }

      // Add to beginning (newest first)
      const updated = [notification, ...prev];

      // Limit total count
      if (updated.length > MAX_NOTIFICATIONS) {
        return updated.slice(0, MAX_NOTIFICATIONS);
      }

      return updated;
    });
  });

  const unreadCount = notifications.filter((n) => !n.read).length;

  const markAsRead = useCallback((id: string) => {
    setNotifications((prev) =>
      prev.map((n) => (n.id === id ? { ...n, read: true } : n))
    );
  }, []);

  const markAllAsRead = useCallback(() => {
    setNotifications((prev) => prev.map((n) => ({ ...n, read: true })));
  }, []);

  const remove = useCallback((id: string) => {
    setNotifications((prev) => prev.filter((n) => n.id !== id));
  }, []);

  const clearAll = useCallback(() => {
    setNotifications([]);
  }, []);

  const addNotification = useCallback(
    (notification: Omit<Notification, 'id' | 'createdAt'>) => {
      const newNotification: Notification = {
        ...notification,
        id: crypto.randomUUID(),
        createdAt: new Date().toISOString(),
      };

      setNotifications((prev) => [newNotification, ...prev]);
    },
    []
  );

  return {
    notifications,
    unreadCount,
    markAsRead,
    markAllAsRead,
    remove,
    clearAll,
    addNotification,
  };
}

// ─────────────────────────────────────────────────────────────────────────────
// NotificationContext - Optional context for global access
// ─────────────────────────────────────────────────────────────────────────────

const NotificationContext = createContext<UseNotificationsReturn | null>(null);

export function NotificationProvider({ children }: { children: ReactNode }) {
  const notifications = useNotifications();

  return (
    <NotificationContext.Provider value={notifications}>
      {children}
    </NotificationContext.Provider>
  );
}

export function useNotificationContext(): UseNotificationsReturn {
  const context = useContext(NotificationContext);
  if (!context) {
    throw new Error(
      'useNotificationContext must be used within a NotificationProvider'
    );
  }
  return context;
}

export default useNotifications;
