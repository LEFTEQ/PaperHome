import { Thermometer, Droplets, Flame, Snowflake, Minus, Plus } from 'lucide-react';
import { motion } from 'framer-motion';
import { GlassCard, BentoCard } from '@/components/ui/glass-card';
import { CircularGauge, CompactGauge } from '@/components/ui/circular-gauge';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { cn } from '@/lib/utils';

export interface TadoRoom {
  id: string;
  name: string;
  currentTemp: number;
  targetTemp: number;
  humidity: number;
  isHeating: boolean;
  mode: 'heat' | 'cool' | 'off' | 'auto';
}

export interface TadoWidgetProps {
  /** Room data */
  room: TadoRoom;
  /** Target temperature change callback */
  onTargetChange?: (target: number) => void;
  /** Bento grid size */
  bentoSize?: '1x1' | '2x1' | '1x2' | '2x2';
  /** Show full gauge or compact */
  variant?: 'full' | 'compact';
  /** Additional className */
  className?: string;
}

export function TadoWidget({
  room,
  onTargetChange,
  bentoSize,
  variant = 'full',
  className,
}: TadoWidgetProps) {
  const CardComponent = bentoSize ? BentoCard : GlassCard;

  const handleIncrement = () => {
    onTargetChange?.(Math.min(30, room.targetTemp + 0.5));
  };

  const handleDecrement = () => {
    onTargetChange?.(Math.max(5, room.targetTemp - 0.5));
  };

  if (variant === 'compact') {
    return (
      <CardComponent
        bentoSize={bentoSize}
        className={cn(
          'flex flex-col',
          room.isHeating && 'border-[hsl(25,95%,53%,0.3)]',
          className
        )}
      >
        {/* Header */}
        <div className="flex items-center justify-between mb-3">
          <div className="flex items-center gap-2">
            <div
              className={cn(
                'h-8 w-8 rounded-lg flex items-center justify-center',
                room.isHeating
                  ? 'bg-[hsl(25,95%,53%,0.2)]'
                  : 'bg-white/[0.05]'
              )}
            >
              {room.isHeating ? (
                <Flame className="h-4 w-4 text-[hsl(25,95%,53%)]" />
              ) : (
                <Thermometer className="h-4 w-4 text-[hsl(228,10%,40%)]" />
              )}
            </div>
            <span className="text-sm font-medium text-white">{room.name}</span>
          </div>
          {room.isHeating && (
            <Badge variant="heating" size="xs">
              Heating
            </Badge>
          )}
        </div>

        {/* Temperature display */}
        <div className="flex items-center justify-between">
          <div>
            <div className="flex items-baseline">
              <span className="text-3xl font-mono font-bold text-white">
                {room.currentTemp.toFixed(1)}
              </span>
              <span className="text-sm text-[hsl(228,10%,50%)] ml-1">°C</span>
            </div>
            <p className="text-xs text-[hsl(228,10%,50%)] mt-1">
              Target: {room.targetTemp.toFixed(1)}°C
            </p>
          </div>

          {/* Quick controls */}
          <div className="flex items-center gap-1">
            <Button
              variant="ghost"
              size="icon-sm"
              onClick={handleDecrement}
              aria-label="Decrease temperature"
            >
              <Minus className="h-4 w-4" />
            </Button>
            <Button
              variant="ghost"
              size="icon-sm"
              onClick={handleIncrement}
              aria-label="Increase temperature"
            >
              <Plus className="h-4 w-4" />
            </Button>
          </div>
        </div>

        {/* Humidity */}
        <div className="flex items-center gap-1 mt-3 text-xs text-[hsl(228,10%,50%)]">
          <Droplets className="h-3.5 w-3.5" />
          <span>{room.humidity}% humidity</span>
        </div>
      </CardComponent>
    );
  }

  // Full variant with circular gauge
  return (
    <CardComponent
      bentoSize={bentoSize}
      className={cn(
        'flex flex-col items-center',
        room.isHeating && 'border-[hsl(25,95%,53%,0.3)]',
        className
      )}
    >
      {/* Header */}
      <div className="w-full flex items-center justify-between mb-4">
        <div className="flex items-center gap-2">
          <div
            className={cn(
              'h-8 w-8 rounded-lg flex items-center justify-center',
              room.isHeating
                ? 'bg-[hsl(25,95%,53%,0.2)]'
                : 'bg-white/[0.05]'
            )}
          >
            {room.isHeating ? (
              <Flame className="h-4 w-4 text-[hsl(25,95%,53%)]" />
            ) : room.mode === 'cool' ? (
              <Snowflake className="h-4 w-4 text-[hsl(199,89%,48%)]" />
            ) : (
              <Thermometer className="h-4 w-4 text-[hsl(228,10%,40%)]" />
            )}
          </div>
          <span className="text-sm font-medium text-white">{room.name}</span>
        </div>
        {room.isHeating && (
          <Badge variant="heating" size="sm" withDot>
            Heating
          </Badge>
        )}
      </div>

      {/* Circular Gauge */}
      <div className="flex-1 flex items-center justify-center">
        <CircularGauge
          current={room.currentTemp}
          target={room.targetTemp}
          isHeating={room.isHeating}
          size={180}
          strokeWidth={10}
        />
      </div>

      {/* Controls */}
      <div className="w-full flex items-center justify-between mt-4">
        <Button
          variant="secondary"
          size="sm"
          onClick={handleDecrement}
          className="gap-1"
        >
          <Minus className="h-4 w-4" />
          <span className="font-mono">0.5°</span>
        </Button>

        <div className="flex items-center gap-1 text-xs text-[hsl(228,10%,50%)]">
          <Droplets className="h-3.5 w-3.5" />
          <span>{room.humidity}%</span>
        </div>

        <Button
          variant="secondary"
          size="sm"
          onClick={handleIncrement}
          className="gap-1"
        >
          <Plus className="h-4 w-4" />
          <span className="font-mono">0.5°</span>
        </Button>
      </div>
    </CardComponent>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Tado Rooms Grid - Multiple rooms
// ─────────────────────────────────────────────────────────────────────────────

export interface TadoRoomsGridProps {
  rooms: TadoRoom[];
  onTargetChange?: (roomId: string, target: number) => void;
  className?: string;
}

export function TadoRoomsGrid({
  rooms,
  onTargetChange,
  className,
}: TadoRoomsGridProps) {
  if (rooms.length === 0) {
    return (
      <GlassCard className={cn('flex items-center justify-center p-8', className)}>
        <div className="text-center">
          <Thermometer className="h-10 w-10 mx-auto text-[hsl(228,10%,30%)] mb-3" />
          <p className="text-sm text-[hsl(228,10%,50%)]">No Tado rooms found</p>
        </div>
      </GlassCard>
    );
  }

  return (
    <div className={cn('grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-4', className)}>
      {rooms.map((room) => (
        <TadoWidget
          key={room.id}
          room={room}
          variant="compact"
          onTargetChange={(target) => onTargetChange?.(room.id, target)}
        />
      ))}
    </div>
  );
}

export default TadoWidget;
