import { Outlet, NavLink, useNavigate } from 'react-router-dom';
import { useAuth } from '@/context/auth-context';
import {
  LayoutDashboard,
  Cpu,
  Settings,
  LogOut,
  Home,
  ChevronRight,
} from 'lucide-react';

const navItems = [
  { to: '/dashboard', icon: LayoutDashboard, label: 'Dashboard' },
  { to: '/devices', icon: Cpu, label: 'Devices' },
  { to: '/settings', icon: Settings, label: 'Settings' },
];

export function MainLayout() {
  const { user, logout } = useAuth();
  const navigate = useNavigate();

  const handleLogout = () => {
    logout();
    navigate('/login');
  };

  return (
    <div className="min-h-screen bg-[hsl(40,20%,96%)]">
      {/* Sidebar */}
      <aside className="fixed left-0 top-0 z-40 h-screen w-64 border-r border-[hsl(30,15%,85%)] bg-[hsl(38,18%,95%)]">
        {/* Paper texture overlay for sidebar */}
        <div
          className="absolute inset-0 opacity-[0.02] pointer-events-none"
          style={{
            backgroundImage: `url("data:image/svg+xml,%3Csvg viewBox='0 0 256 256' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noise'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.9' numOctaves='4' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23noise)'/%3E%3C/svg%3E")`,
          }}
        />

        <div className="flex h-full flex-col relative">
          {/* Logo */}
          <div className="flex h-16 items-center gap-3 border-b border-[hsl(30,15%,88%)] px-5">
            <div className="flex h-9 w-9 items-center justify-center rounded-lg bg-[hsl(25,60%,45%)]">
              <Home className="h-5 w-5 text-white" />
            </div>
            <div>
              <h1 className="font-semibold text-[hsl(30,10%,15%)] tracking-tight">
                PaperHome
              </h1>
              <p className="text-xs text-[hsl(30,10%,50%)]">Smart Dashboard</p>
            </div>
          </div>

          {/* Navigation */}
          <nav className="flex-1 space-y-1 px-3 py-4">
            {navItems.map((item) => (
              <NavLink
                key={item.to}
                to={item.to}
                className={({ isActive }) =>
                  `group flex items-center gap-3 rounded-lg px-3 py-2.5 text-sm font-medium transition-all duration-150 ${
                    isActive
                      ? 'bg-[hsl(35,15%,92%)] text-[hsl(25,60%,40%)] shadow-sm'
                      : 'text-[hsl(30,10%,40%)] hover:bg-[hsl(35,15%,93%)] hover:text-[hsl(30,10%,25%)]'
                  }`
                }
              >
                <item.icon className="h-5 w-5 flex-shrink-0" />
                <span>{item.label}</span>
                <ChevronRight className="ml-auto h-4 w-4 opacity-0 group-hover:opacity-50 transition-opacity" />
              </NavLink>
            ))}
          </nav>

          {/* User section */}
          <div className="border-t border-[hsl(30,15%,88%)] p-3">
            <div className="flex items-center gap-3 rounded-lg bg-[hsl(35,15%,93%)] px-3 py-2.5">
              <div className="flex h-9 w-9 items-center justify-center rounded-full bg-[hsl(140,30%,35%)] text-white text-sm font-medium">
                {user?.displayName?.charAt(0).toUpperCase() || 'U'}
              </div>
              <div className="flex-1 min-w-0">
                <p className="text-sm font-medium text-[hsl(30,10%,20%)] truncate">
                  {user?.displayName}
                </p>
                <p className="text-xs text-[hsl(30,10%,50%)] truncate">
                  @{user?.username}
                </p>
              </div>
              <button
                onClick={handleLogout}
                className="rounded-md p-1.5 text-[hsl(30,10%,50%)] hover:bg-[hsl(30,10%,88%)] hover:text-[hsl(0,50%,45%)] transition-colors"
                title="Logout"
              >
                <LogOut className="h-4 w-4" />
              </button>
            </div>
          </div>
        </div>
      </aside>

      {/* Main content */}
      <main className="pl-64">
        <div className="min-h-screen">
          <Outlet />
        </div>
      </main>
    </div>
  );
}
