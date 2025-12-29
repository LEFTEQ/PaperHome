import { HTMLAttributes } from 'react';
import { cn } from '@/lib/utils';

export interface SkeletonProps extends HTMLAttributes<HTMLDivElement> {
  /** Animation type */
  animation?: 'pulse' | 'shimmer' | 'none';
}

function Skeleton({
  className,
  animation = 'shimmer',
  ...props
}: SkeletonProps) {
  return (
    <div
      className={cn(
        'rounded-lg bg-glass-bright',
        {
          'animate-pulse': animation === 'pulse',
          'skeleton': animation === 'shimmer',
        },
        className
      )}
      {...props}
    />
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Pre-built skeleton patterns
// ─────────────────────────────────────────────────────────────────────────────

/** Skeleton for text lines */
export function SkeletonText({
  lines = 3,
  className,
}: {
  lines?: number;
  className?: string;
}) {
  return (
    <div className={cn('space-y-2', className)}>
      {Array.from({ length: lines }).map((_, i) => (
        <Skeleton
          key={i}
          className={cn('h-4', i === lines - 1 && 'w-3/4')}
        />
      ))}
    </div>
  );
}

/** Skeleton for card content */
export function SkeletonCard({ className }: { className?: string }) {
  return (
    <div
      className={cn(
        'rounded-2xl border border-glass-border bg-glass p-5',
        className
      )}
    >
      <div className="flex items-center gap-4 mb-4">
        <Skeleton className="h-10 w-10 rounded-full" />
        <div className="flex-1 space-y-2">
          <Skeleton className="h-4 w-1/3" />
          <Skeleton className="h-3 w-1/4" />
        </div>
      </div>
      <SkeletonText lines={2} />
    </div>
  );
}

/** Skeleton for device summary card */
export function SkeletonDeviceCard({ className }: { className?: string }) {
  return (
    <div
      className={cn(
        'rounded-2xl border border-glass-border bg-glass p-5',
        className
      )}
    >
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center gap-3">
          <Skeleton className="h-2.5 w-2.5 rounded-full" />
          <Skeleton className="h-5 w-32" />
        </div>
        <Skeleton className="h-6 w-16 rounded-full" />
      </div>
      <div className="grid grid-cols-2 gap-4">
        <div className="space-y-1">
          <Skeleton className="h-3 w-8" />
          <Skeleton className="h-8 w-20" />
        </div>
        <div className="space-y-1">
          <Skeleton className="h-3 w-8" />
          <Skeleton className="h-8 w-16" />
        </div>
        <div className="space-y-1">
          <Skeleton className="h-3 w-12" />
          <Skeleton className="h-8 w-14" />
        </div>
        <div className="space-y-1">
          <Skeleton className="h-3 w-10" />
          <Skeleton className="h-8 w-12" />
        </div>
      </div>
    </div>
  );
}

/** Skeleton for stat widget (bento card) */
export function SkeletonStatCard({ className }: { className?: string }) {
  return (
    <div
      className={cn(
        'rounded-2xl border border-glass-border bg-glass p-5 flex flex-col',
        className
      )}
    >
      <div className="flex items-center justify-between mb-4">
        <Skeleton className="h-4 w-20" />
        <Skeleton className="h-8 w-8 rounded-lg" />
      </div>
      <div className="flex-1 flex items-end justify-between">
        <div className="space-y-1">
          <Skeleton className="h-12 w-24" />
          <Skeleton className="h-3 w-16" />
        </div>
        <Skeleton className="h-16 w-24 rounded-lg" />
      </div>
    </div>
  );
}

/** Skeleton for dashboard grid */
export function SkeletonDashboard({ className }: { className?: string }) {
  return (
    <div className={cn('space-y-6', className)}>
      {/* Header */}
      <div className="flex items-center justify-between">
        <Skeleton className="h-8 w-48" />
        <Skeleton className="h-10 w-32 rounded-lg" />
      </div>

      {/* Summary stats */}
      <div className="grid grid-cols-1 md:grid-cols-3 gap-4">
        {Array.from({ length: 3 }).map((_, i) => (
          <div
            key={i}
            className="rounded-2xl border border-glass-border bg-glass p-4 flex items-center gap-4"
          >
            <Skeleton className="h-10 w-10 rounded-lg" />
            <div className="space-y-1">
              <Skeleton className="h-3 w-16" />
              <Skeleton className="h-6 w-20" />
            </div>
          </div>
        ))}
      </div>

      {/* Device grid */}
      <div className="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4">
        {Array.from({ length: 6 }).map((_, i) => (
          <SkeletonDeviceCard key={i} />
        ))}
      </div>
    </div>
  );
}

/** Skeleton for bento grid */
export function SkeletonBentoGrid({ className }: { className?: string }) {
  return (
    <div className={cn('bento-grid', className)}>
      <SkeletonStatCard className="bento-2x2" />
      <SkeletonStatCard className="bento-1x1" />
      <SkeletonStatCard className="bento-1x1" />
      <SkeletonStatCard className="bento-2x1" />
      <SkeletonStatCard className="bento-2x2" />
    </div>
  );
}

/** Skeleton for circular gauge */
export function SkeletonGauge({
  className,
  size = 'md',
}: {
  className?: string;
  size?: 'sm' | 'md' | 'lg';
}) {
  const sizeMap = {
    sm: 'h-24 w-24',
    md: 'h-32 w-32',
    lg: 'h-40 w-40',
  };

  return (
    <div className={cn('flex flex-col items-center gap-3', className)}>
      <Skeleton className={cn('rounded-full', sizeMap[size])} />
      <div className="text-center space-y-1">
        <Skeleton className="h-6 w-20 mx-auto" />
        <Skeleton className="h-3 w-16 mx-auto" />
      </div>
    </div>
  );
}

/** Skeleton for chart */
export function SkeletonChart({
  className,
  height = 200,
}: {
  className?: string;
  height?: number;
}) {
  return (
    <div className={cn('w-full', className)}>
      <Skeleton style={{ height }} className="w-full rounded-lg" />
    </div>
  );
}

export { Skeleton };
