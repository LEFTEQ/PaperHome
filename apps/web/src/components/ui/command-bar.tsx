import { useState, useRef, useEffect, useCallback, useMemo } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Search,
  Command,
  X,
  Cpu,
  Lightbulb,
  Thermometer,
  ArrowRight,
  Zap,
} from 'lucide-react';
import { cn } from '@/lib/utils';
import { dropdownAnimation, fadeInUp } from '@/lib/animations';

export interface CommandItem {
  id: string;
  type: 'device' | 'action' | 'navigation';
  label: string;
  description?: string;
  icon?: React.ReactNode;
  keywords?: string[];
  onSelect: () => void;
}

export interface CommandBarProps {
  /** Placeholder text */
  placeholder?: string;
  /** Command items to search through */
  items: CommandItem[];
  /** Callback when search value changes */
  onSearchChange?: (value: string) => void;
  /** Show keyboard shortcut hint */
  showShortcut?: boolean;
  /** Expanded by default (for desktop) */
  defaultExpanded?: boolean;
}

const typeIcons = {
  device: Cpu,
  action: Zap,
  navigation: ArrowRight,
};

const typeColors = {
  device: 'text-accent',
  action: 'text-warning',
  navigation: 'text-success',
};

export function CommandBar({
  placeholder = 'Search devices, actions...',
  items,
  onSearchChange,
  showShortcut = true,
  defaultExpanded = false,
}: CommandBarProps) {
  const [isOpen, setIsOpen] = useState(false);
  const [isExpanded, setIsExpanded] = useState(defaultExpanded);
  const [query, setQuery] = useState('');
  const [selectedIndex, setSelectedIndex] = useState(0);
  const inputRef = useRef<HTMLInputElement>(null);
  const listRef = useRef<HTMLDivElement>(null);

  // Filter items based on query
  const filteredItems = useMemo(() => {
    if (!query.trim()) return items.slice(0, 5); // Show first 5 as suggestions

    const lowerQuery = query.toLowerCase();
    return items.filter((item) => {
      const matchLabel = item.label.toLowerCase().includes(lowerQuery);
      const matchDesc = item.description?.toLowerCase().includes(lowerQuery);
      const matchKeywords = item.keywords?.some((k) =>
        k.toLowerCase().includes(lowerQuery)
      );
      return matchLabel || matchDesc || matchKeywords;
    });
  }, [items, query]);

  // Handle keyboard shortcut (Cmd+K / Ctrl+K)
  useEffect(() => {
    const handleKeyDown = (e: KeyboardEvent) => {
      if ((e.metaKey || e.ctrlKey) && e.key === 'k') {
        e.preventDefault();
        setIsExpanded(true);
        setIsOpen(true);
        setTimeout(() => inputRef.current?.focus(), 0);
      }

      if (e.key === 'Escape') {
        setIsOpen(false);
        setQuery('');
        inputRef.current?.blur();
      }
    };

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, []);

  // Handle arrow key navigation
  const handleKeyDown = useCallback(
    (e: React.KeyboardEvent) => {
      switch (e.key) {
        case 'ArrowDown':
          e.preventDefault();
          setSelectedIndex((prev) =>
            prev < filteredItems.length - 1 ? prev + 1 : 0
          );
          break;
        case 'ArrowUp':
          e.preventDefault();
          setSelectedIndex((prev) =>
            prev > 0 ? prev - 1 : filteredItems.length - 1
          );
          break;
        case 'Enter':
          e.preventDefault();
          if (filteredItems[selectedIndex]) {
            filteredItems[selectedIndex].onSelect();
            setIsOpen(false);
            setQuery('');
          }
          break;
      }
    },
    [filteredItems, selectedIndex]
  );

  // Scroll selected item into view
  useEffect(() => {
    if (listRef.current) {
      const selectedElement = listRef.current.children[selectedIndex] as HTMLElement;
      if (selectedElement) {
        selectedElement.scrollIntoView({ block: 'nearest' });
      }
    }
  }, [selectedIndex]);

  // Reset selection when query changes
  useEffect(() => {
    setSelectedIndex(0);
  }, [query]);

  const handleFocus = () => {
    setIsExpanded(true);
    setIsOpen(true);
  };

  const handleBlur = (e: React.FocusEvent) => {
    // Don't close if clicking inside the dropdown
    if (e.relatedTarget && e.currentTarget.contains(e.relatedTarget as Node)) {
      return;
    }
    // Delay closing to allow item selection
    setTimeout(() => {
      setIsOpen(false);
      if (!defaultExpanded) {
        setIsExpanded(false);
      }
    }, 150);
  };

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const value = e.target.value;
    setQuery(value);
    onSearchChange?.(value);
  };

  const handleClear = () => {
    setQuery('');
    inputRef.current?.focus();
  };

  return (
    <div className="relative" onBlur={handleBlur}>
      {/* Input container */}
      <div
        className={cn(
          'relative flex items-center',
          'rounded-xl border border-glass-border-hover',
          'bg-glass backdrop-blur-sm',
          'transition-all duration-200',
          isExpanded ? 'w-80' : 'w-10',
          isOpen && 'border-accent/30 shadow-[0_0_15px_rgb(0_229_255/0.1)]'
        )}
      >
        {/* Search icon / button */}
        <button
          onClick={() => {
            setIsExpanded(true);
            setTimeout(() => inputRef.current?.focus(), 0);
          }}
          className={cn(
            'flex-shrink-0 p-2.5',
            'text-text-muted',
            isExpanded && 'pointer-events-none'
          )}
        >
          <Search className="h-4 w-4" />
        </button>

        {/* Input */}
        <AnimatePresence>
          {isExpanded && (
            <motion.input
              ref={inputRef}
              initial={{ opacity: 0, width: 0 }}
              animate={{ opacity: 1, width: 'auto' }}
              exit={{ opacity: 0, width: 0 }}
              type="text"
              value={query}
              onChange={handleChange}
              onFocus={handleFocus}
              onKeyDown={handleKeyDown}
              placeholder={placeholder}
              className={cn(
                'flex-1 bg-transparent border-none outline-none',
                'text-sm text-white placeholder:text-text-subtle',
                'pr-2'
              )}
            />
          )}
        </AnimatePresence>

        {/* Clear button / Shortcut hint */}
        {isExpanded && (
          <div className="flex-shrink-0 pr-2 flex items-center gap-1">
            {query ? (
              <button
                onClick={handleClear}
                className="p-1 rounded-md text-text-subtle hover:text-white hover:bg-glass-hover"
              >
                <X className="h-3.5 w-3.5" />
              </button>
            ) : showShortcut ? (
              <kbd
                className={cn(
                  'hidden sm:flex items-center gap-0.5 px-1.5 py-0.5 rounded',
                  'bg-glass-hover border border-glass-border-hover',
                  'text-[10px] text-text-subtle font-mono'
                )}
              >
                <Command className="h-2.5 w-2.5" />K
              </kbd>
            ) : null}
          </div>
        )}
      </div>

      {/* Results dropdown */}
      <AnimatePresence>
        {isOpen && filteredItems.length > 0 && (
          <motion.div
            className={cn(
              'absolute top-full left-0 right-0 mt-2 z-50',
              'rounded-xl border border-glass-border-hover',
              'bg-bg-elevated/95 backdrop-blur-xl',
              'shadow-2xl overflow-hidden'
            )}
            {...dropdownAnimation}
          >
            <div
              ref={listRef}
              className="py-1 max-h-[300px] overflow-y-auto"
            >
              {filteredItems.map((item, index) => {
                const TypeIcon = item.icon ? null : typeIcons[item.type];

                return (
                  <motion.button
                    key={item.id}
                    variants={fadeInUp}
                    onClick={() => {
                      item.onSelect();
                      setIsOpen(false);
                      setQuery('');
                    }}
                    className={cn(
                      'w-full flex items-center gap-3 px-3 py-2',
                      'text-left transition-colors duration-100',
                      index === selectedIndex
                        ? 'bg-glass-hover text-white'
                        : 'text-text-secondary hover:bg-glass'
                    )}
                  >
                    {/* Icon */}
                    <div
                      className={cn(
                        'flex-shrink-0 h-8 w-8 rounded-lg',
                        'bg-glass-hover border border-glass-border-hover',
                        'flex items-center justify-center',
                        typeColors[item.type]
                      )}
                    >
                      {item.icon || (TypeIcon && <TypeIcon className="h-4 w-4" />)}
                    </div>

                    {/* Content */}
                    <div className="flex-1 min-w-0">
                      <p className="text-sm font-medium truncate">{item.label}</p>
                      {item.description && (
                        <p className="text-xs text-text-muted truncate">
                          {item.description}
                        </p>
                      )}
                    </div>

                    {/* Type badge */}
                    <span
                      className={cn(
                        'flex-shrink-0 px-2 py-0.5 rounded text-[10px] uppercase',
                        'bg-glass-hover text-text-muted'
                      )}
                    >
                      {item.type}
                    </span>
                  </motion.button>
                );
              })}
            </div>

            {/* Footer hint */}
            <div className="px-3 py-2 border-t border-glass-border flex items-center justify-between text-[10px] text-text-subtle">
              <span>
                <kbd className="px-1 py-0.5 rounded bg-glass-hover mr-1">↑↓</kbd>
                navigate
              </span>
              <span>
                <kbd className="px-1 py-0.5 rounded bg-glass-hover mr-1">↵</kbd>
                select
              </span>
              <span>
                <kbd className="px-1 py-0.5 rounded bg-glass-hover mr-1">esc</kbd>
                close
              </span>
            </div>
          </motion.div>
        )}
      </AnimatePresence>

      {/* No results */}
      <AnimatePresence>
        {isOpen && query && filteredItems.length === 0 && (
          <motion.div
            className={cn(
              'absolute top-full left-0 right-0 mt-2 z-50',
              'rounded-xl border border-glass-border-hover',
              'bg-bg-elevated/95 backdrop-blur-xl',
              'shadow-2xl p-6 text-center'
            )}
            {...dropdownAnimation}
          >
            <Search className="h-8 w-8 mx-auto text-text-subtle mb-2" />
            <p className="text-sm text-text-muted">
              No results for "{query}"
            </p>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

export default CommandBar;
