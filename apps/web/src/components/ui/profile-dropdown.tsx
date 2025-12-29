import { User, Settings, LogOut, Moon, Sun, Monitor } from 'lucide-react';
import {
  DropdownMenu,
  DropdownMenuContent,
  DropdownMenuGroup,
  DropdownMenuItem,
  DropdownMenuLabel,
  DropdownMenuSeparator,
  DropdownMenuTrigger,
} from './dropdown';
import { cn } from '@/lib/utils';

export interface ProfileDropdownProps {
  user: {
    username: string;
    displayName?: string;
    email?: string;
  };
  onSettings?: () => void;
  onLogout: () => void;
}

export function ProfileDropdown({
  user,
  onSettings,
  onLogout,
}: ProfileDropdownProps) {
  const initials = (user.displayName || user.username)
    .split(' ')
    .map((n) => n[0])
    .join('')
    .toUpperCase()
    .slice(0, 2);

  return (
    <DropdownMenu>
      <DropdownMenuTrigger asChild>
        <button
          className={cn(
            'flex items-center gap-2 p-1.5 rounded-xl',
            'hover:bg-glass-hover',
            'transition-colors duration-200',
            'focus:outline-none focus:ring-2 focus:ring-accent/50'
          )}
        >
          {/* Avatar */}
          <div
            className={cn(
              'h-8 w-8 rounded-lg',
              'bg-gradient-to-br from-accent to-success',
              'flex items-center justify-center',
              'text-xs font-bold text-bg-base'
            )}
          >
            {initials}
          </div>
        </button>
      </DropdownMenuTrigger>

      <DropdownMenuContent align="end" className="w-56" sideOffset={12}>
        {/* User info */}
        <div className="px-3 py-3 border-b border-glass-border">
          <p className="text-sm font-medium text-white">
            {user.displayName || user.username}
          </p>
          <p className="text-xs text-text-muted mt-0.5">
            @{user.username}
          </p>
        </div>

        <DropdownMenuGroup className="py-1">
          {onSettings && (
            <DropdownMenuItem onClick={onSettings}>
              <Settings className="h-4 w-4" />
              <span>Settings</span>
            </DropdownMenuItem>
          )}
        </DropdownMenuGroup>

        <DropdownMenuSeparator />

        <DropdownMenuGroup className="py-1">
          <DropdownMenuItem onClick={onLogout} destructive>
            <LogOut className="h-4 w-4" />
            <span>Log out</span>
          </DropdownMenuItem>
        </DropdownMenuGroup>
      </DropdownMenuContent>
    </DropdownMenu>
  );
}

export default ProfileDropdown;
