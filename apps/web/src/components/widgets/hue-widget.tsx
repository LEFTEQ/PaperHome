import { Lightbulb, LightbulbOff } from 'lucide-react';
import { motion } from 'framer-motion';
import { GlassCard, BentoCard } from '@/components/ui/glass-card';
import { Toggle } from '@/components/ui/toggle';
import { BrightnessSlider } from '@/components/ui/slider';
import { Badge } from '@/components/ui/badge';
import { cn } from '@/lib/utils';

export interface HueRoom {
  id: string;
  name: string;
  isOn: boolean;
  brightness: number; // 0-100
}

export interface HueWidgetProps {
  /** Room data */
  room: HueRoom;
  /** Toggle callback */
  onToggle?: (isOn: boolean) => void;
  /** Brightness change callback */
  onBrightnessChange?: (brightness: number) => void;
  /** Brightness change end callback (for API calls) */
  onBrightnessChangeEnd?: (brightness: number) => void;
  /** Bento grid size */
  bentoSize?: '1x1' | '2x1' | '1x2' | '2x2';
  /** Compact mode - no brightness slider */
  compact?: boolean;
  /** Additional className */
  className?: string;
}

export function HueWidget({
  room,
  onToggle,
  onBrightnessChange,
  onBrightnessChangeEnd,
  bentoSize,
  compact = false,
  className,
}: HueWidgetProps) {
  const CardComponent = bentoSize ? BentoCard : GlassCard;

  return (
    <CardComponent
      bentoSize={bentoSize}
      className={cn(
        'flex flex-col',
        room.isOn && 'border-warning/30',
        className
      )}
      style={{
        background: room.isOn
          ? `linear-gradient(135deg, hsla(38, 92%, 50%, ${room.brightness / 1000}) 0%, transparent 50%), rgba(255, 255, 255, 0.025)`
          : undefined,
      }}
    >
      {/* Header */}
      <div className="flex items-center justify-between mb-4">
        <div className="flex items-center gap-3">
          <motion.div
            className={cn(
              'h-10 w-10 rounded-xl flex items-center justify-center',
              room.isOn
                ? 'bg-warning/20'
                : 'bg-glass-hover'
            )}
            animate={{
              boxShadow: room.isOn
                ? '0 0 20px hsla(38, 92%, 50%, 0.3)'
                : 'none',
            }}
          >
            {room.isOn ? (
              <Lightbulb
                className="h-5 w-5 text-warning"
              />
            ) : (
              <LightbulbOff className="h-5 w-5 text-text-subtle" />
            )}
          </motion.div>
          <div>
            <h3 className="text-sm font-medium text-white">{room.name}</h3>
            <p className="text-xs text-text-muted">
              {room.isOn ? `${room.brightness}% brightness` : 'Off'}
            </p>
          </div>
        </div>

        <Toggle
          checked={room.isOn}
          onCheckedChange={onToggle}
          size="sm"
          variant="default"
        />
      </div>

      {/* Brightness slider */}
      {!compact && room.isOn && (
        <motion.div
          initial={{ opacity: 0, height: 0 }}
          animate={{ opacity: 1, height: 'auto' }}
          exit={{ opacity: 0, height: 0 }}
          transition={{ duration: 0.2 }}
          className="mt-auto"
        >
          <BrightnessSlider
            value={room.brightness}
            onChange={onBrightnessChange}
            onChangeEnd={onBrightnessChangeEnd}
          />
        </motion.div>
      )}

      {/* Compact brightness indicator */}
      {compact && room.isOn && (
        <div className="mt-auto">
          <div className="h-1.5 rounded-full bg-white/[0.1] overflow-hidden">
            <motion.div
              className="h-full rounded-full bg-warning"
              initial={{ width: 0 }}
              animate={{ width: `${room.brightness}%` }}
              transition={{ duration: 0.5 }}
              style={{
                boxShadow: '0 0 10px hsla(38, 92%, 50%, 0.5)',
              }}
            />
          </div>
        </div>
      )}
    </CardComponent>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Hue Rooms Grid - Multiple rooms in a grid
// ─────────────────────────────────────────────────────────────────────────────

export interface HueRoomsGridProps {
  rooms: HueRoom[];
  onToggle?: (roomId: string, isOn: boolean) => void;
  onBrightnessChange?: (roomId: string, brightness: number) => void;
  onBrightnessChangeEnd?: (roomId: string, brightness: number) => void;
  className?: string;
}

export function HueRoomsGrid({
  rooms,
  onToggle,
  onBrightnessChange,
  onBrightnessChangeEnd,
  className,
}: HueRoomsGridProps) {
  if (rooms.length === 0) {
    return (
      <GlassCard className={cn('flex items-center justify-center p-8', className)}>
        <div className="text-center">
          <LightbulbOff className="h-10 w-10 mx-auto text-text-subtle mb-3" />
          <p className="text-sm text-text-muted">No Hue rooms found</p>
        </div>
      </GlassCard>
    );
  }

  return (
    <div className={cn('grid grid-cols-1 sm:grid-cols-2 gap-4', className)}>
      {rooms.map((room) => (
        <HueWidget
          key={room.id}
          room={room}
          onToggle={(isOn) => onToggle?.(room.id, isOn)}
          onBrightnessChange={(brightness) =>
            onBrightnessChange?.(room.id, brightness)
          }
          onBrightnessChangeEnd={(brightness) =>
            onBrightnessChangeEnd?.(room.id, brightness)
          }
          compact
        />
      ))}
    </div>
  );
}

export default HueWidget;
