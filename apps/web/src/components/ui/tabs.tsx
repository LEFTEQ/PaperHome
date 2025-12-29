import { createContext, useContext, useState, ReactNode } from 'react';
import { cn } from '@/lib/utils';

interface TabsContextValue {
  activeTab: string;
  setActiveTab: (value: string) => void;
}

const TabsContext = createContext<TabsContextValue | undefined>(undefined);

function useTabs() {
  const context = useContext(TabsContext);
  if (!context) {
    throw new Error('Tabs components must be used within a Tabs provider');
  }
  return context;
}

interface TabsProps {
  defaultValue: string;
  children: ReactNode;
  className?: string;
  onValueChange?: (value: string) => void;
}

function Tabs({ defaultValue, children, className, onValueChange }: TabsProps) {
  const [activeTab, setActiveTab] = useState(defaultValue);

  const handleSetActiveTab = (value: string) => {
    setActiveTab(value);
    onValueChange?.(value);
  };

  return (
    <TabsContext.Provider value={{ activeTab, setActiveTab: handleSetActiveTab }}>
      <div className={className}>{children}</div>
    </TabsContext.Provider>
  );
}

interface TabsListProps {
  children: ReactNode;
  className?: string;
}

function TabsList({ children, className }: TabsListProps) {
  return (
    <div
      className={cn(
        'inline-flex h-10 items-center justify-center rounded-lg p-1',
        'bg-[hsl(30,10%,90%)] border border-[hsl(30,15%,85%)]',
        className
      )}
      role="tablist"
    >
      {children}
    </div>
  );
}

interface TabsTriggerProps {
  value: string;
  children: ReactNode;
  className?: string;
}

function TabsTrigger({ value, children, className }: TabsTriggerProps) {
  const { activeTab, setActiveTab } = useTabs();
  const isActive = activeTab === value;

  return (
    <button
      role="tab"
      aria-selected={isActive}
      onClick={() => setActiveTab(value)}
      className={cn(
        'inline-flex items-center justify-center whitespace-nowrap rounded-md px-3 py-1.5',
        'text-sm font-medium transition-all duration-200',
        'focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-[hsl(25,60%,45%)] focus-visible:ring-offset-1',
        'disabled:pointer-events-none disabled:opacity-50',
        isActive
          ? 'bg-[hsl(35,15%,96%)] text-[hsl(30,10%,15%)] shadow-sm'
          : 'text-[hsl(30,10%,45%)] hover:text-[hsl(30,10%,25%)] hover:bg-[hsl(30,10%,92%)]',
        className
      )}
    >
      {children}
    </button>
  );
}

interface TabsContentProps {
  value: string;
  children: ReactNode;
  className?: string;
}

function TabsContent({ value, children, className }: TabsContentProps) {
  const { activeTab } = useTabs();

  if (activeTab !== value) return null;

  return (
    <div
      role="tabpanel"
      className={cn(
        'mt-4 focus-visible:outline-none',
        'animate-in fade-in-0 duration-200',
        className
      )}
    >
      {children}
    </div>
  );
}

export { Tabs, TabsList, TabsTrigger, TabsContent };
