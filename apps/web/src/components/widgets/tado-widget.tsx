import { Thermometer, Droplets, Flame, Snowflake, Minus, Plus, Zap, AlertTriangle } from 'lucide-react';
import { motion } from 'framer-motion';
import { GlassCard, BentoCard } from '@/components/ui/glass-card';
import { CircularGauge } from '@/components/ui/circular-gauge';
import { InteractiveGauge } from '@/components/ui/interactive-gauge';
import { Badge } from '@/components/ui/badge';
import { Button } from '@/components/ui/button';
import { Toggle } from '@/components/ui/toggle';
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

export interface TadoZoneMappingData {
  id: string;
  tadoZoneId: number;
  tadoZoneName: string;
  targetTemperature: number;
  autoAdjustEnabled: boolean;
  hysteresis: number;
}

export interface TadoWidgetProps {
  /** Room data */
  room: TadoRoom;
  /** Zone mapping data (for auto-adjust feature) */
  zoneMapping?: TadoZoneMappingData;
  /** ESP32 sensor temperature (for auto-adjust display) */
  esp32Temp?: number;
  /** Is device online */
  isDeviceOnline?: boolean;
  /** Target temperature change callback */
  onTargetChange?: (target: number) => void;
  /** Auto-adjust toggle callback */
  onAutoAdjustToggle?: (enabled: boolean) => void;
  /** Bento grid size */
  bentoSize?: '1x1' | '2x1' | '1x2' | '2x2';
  /** Show full gauge or compact */
  variant?: 'full' | 'compact' | 'interactive';
  /** Additional className */
  className?: string;
}

export function TadoWidget({
  room,
  zoneMapping,
  esp32Temp,
  isDeviceOnline = true,
  onTargetChange,
  onAutoAdjustToggle,
  bentoSize,
  variant = 'full',
  className,
}: TadoWidgetProps) {
  const CardComponent = bentoSize ? BentoCard : GlassCard;
  const isAutoAdjust = zoneMapping?.autoAdjustEnabled ?? false;

  const handleIncrement = () => {
    onTargetChange?.(Math.min(25, room.targetTemp + 0.5));
  };

  const handleDecrement = () => {
    onTargetChange?.(Math.max(5, room.targetTemp - 0.5));
  };

  // Get status badge
  const getStatusBadge = () => {
    if (!isDeviceOnline) {
      return (
        <Badge variant="warning" size="xs">
          <AlertTriangle className="h-3 w-3 mr-1" />
          Offline
        </Badge>
      );
    }
    if (isAutoAdjust) {
      return (
        <Badge variant="accent" size="xs" withDot>
          Auto
        </Badge>
      );
    }
    if (room.isHeating) {
      return (
        <Badge variant="heating" size="xs" withDot>
          Heating
        </Badge>
      );
    }
    return null;
  };

  // Interactive variant with draggable gauge
  if (variant === 'interactive') {
    const displayTemp = esp32Temp ?? room.currentTemp;
    const targetTemp = zoneMapping?.targetTemperature ?? room.targetTemp;

    return (
      <CardComponent
        bentoSize={bentoSize}
        className={cn(
          'flex flex-col items-center',
          room.isHeating && isDeviceOnline && 'tado-heating-bg',
          isAutoAdjust && isDeviceOnline && 'tado-auto-adjust',
          !isDeviceOnline && 'opacity-60',
          className
        )}
      >
        {/* Header */}
        <div className="w-full flex items-center justify-between mb-4">
          <div className="flex items-center gap-2">
            <div
              className={cn(
                'h-8 w-8 rounded-lg flex items-center justify-center',
                room.isHeating && isDeviceOnline
                  ? 'bg-heating/20'
                  : isAutoAdjust && isDeviceOnline
                  ? 'bg-accent/20'
                  : 'bg-glass-hover'
              )}
            >
              {room.isHeating && isDeviceOnline ? (
                <Flame className="h-4 w-4 text-heating" />
              ) : isAutoAdjust && isDeviceOnline ? (
                <Zap className="h-4 w-4 text-accent" />
              ) : (
                <Thermometer className="h-4 w-4 text-text-subtle" />
              )}
            </div>
            <span className="text-sm font-medium text-white">{room.name}</span>
          </div>
          {getStatusBadge()}
        </div>

        {/* Interactive Gauge */}
        <div className="flex-1 flex items-center justify-center">
          <InteractiveGauge
            currentTemp={displayTemp}
            targetTemp={targetTemp}
            tadoTarget={room.targetTemp}
            isHeating={room.isHeating}
            isAutoAdjust={isAutoAdjust}
            isOffline={!isDeviceOnline}
            size={180}
            strokeWidth={10}
            onTargetChange={onTargetChange}
          />
        </div>

        {/* Footer with auto-adjust toggle and humidity */}
        <div className="w-full flex items-center justify-between mt-4">
          <div className="flex items-center gap-1 text-xs text-text-muted">
            <Droplets className="h-3.5 w-3.5" />
            <span>{room.humidity}%</span>
          </div>

          {onAutoAdjustToggle && (
            <div className="flex items-center gap-2">
              <span className="text-xs text-text-muted">Auto</span>
              <Toggle
                size="sm"
                variant="default"
                checked={isAutoAdjust}
                onCheckedChange={onAutoAdjustToggle}
                disabled={!isDeviceOnline}
              />
            </div>
          )}
        </div>
      </CardComponent>
    );
  }

  if (variant === 'compact') {
    return (
      <CardComponent
        bentoSize={bentoSize}
        className={cn(
          'flex flex-col',
          room.isHeating && 'border-heating/30',
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
                  ? 'bg-heating/20'
                  : 'bg-glass-hover'
              )}
            >
              {room.isHeating ? (
                <Flame className="h-4 w-4 text-heating" />
              ) : (
                <Thermometer className="h-4 w-4 text-text-subtle" />
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
              <span className="text-sm text-text-muted ml-1">°C</span>
            </div>
            <p className="text-xs text-text-muted mt-1">
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
        <div className="flex items-center gap-1 mt-3 text-xs text-text-muted">
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
        room.isHeating && 'border-heating/30',
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
                ? 'bg-heating/20'
                : 'bg-glass-hover'
            )}
          >
            {room.isHeating ? (
              <Flame className="h-4 w-4 text-heating" />
            ) : room.mode === 'cool' ? (
              <Snowflake className="h-4 w-4 text-info" />
            ) : (
              <Thermometer className="h-4 w-4 text-text-subtle" />
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

        <div className="flex items-center gap-1 text-xs text-text-muted">
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
          <Thermometer className="h-10 w-10 mx-auto text-text-subtle mb-3" />
          <p className="text-sm text-text-muted">No Tado rooms found</p>
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
