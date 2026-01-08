import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import {
  Thermometer,
  Plus,
  Trash2,
  Zap,
  Settings,
  ChevronDown,
  ChevronUp,
  AlertTriangle,
  ArrowRight,
  Info,
  Gauge,
} from 'lucide-react';
import { GlassCard } from '@/components/ui/glass-card';
import { Button } from '@/components/ui/button';
import { Toggle } from '@/components/ui/toggle';
import { Badge } from '@/components/ui/badge';
import { Slider } from '@/components/ui/slider';
import { cn } from '@/lib/utils';
import {
  useZoneMappings,
  useCreateZoneMapping,
  useUpdateZoneMapping,
  useDeleteZoneMapping,
  useToggleAutoAdjust,
  useSetTargetTemperature,
} from '@/hooks/use-zone-mappings';
import { TadoRoom, TadoZoneMapping } from '@/lib/api';

export interface ZoneMappingSectionProps {
  /** Device ID */
  deviceId: string;
  /** Device name */
  deviceName?: string;
  /** Is device online */
  isDeviceOnline?: boolean;
  /** Available Tado zones from the device */
  availableZones?: TadoRoom[];
  /** Current ESP32 sensor temperature */
  esp32Temp?: number;
  /** Additional className */
  className?: string;
}

/**
 * Zone Mapping Section for Device Detail Page
 *
 * Allows users to:
 * - View existing zone mappings
 * - Add new zone mappings
 * - Toggle auto-adjust per zone
 * - Set target temperature per zone
 * - Remove zone mappings
 */
export function ZoneMappingSection({
  deviceId,
  deviceName,
  isDeviceOnline = true,
  availableZones = [],
  esp32Temp,
  className,
}: ZoneMappingSectionProps) {
  const [isExpanded, setIsExpanded] = useState(true);
  const [showAddZone, setShowAddZone] = useState(false);
  const [showHelp, setShowHelp] = useState(false);

  // Fetch zone mappings
  const { data: mappings = [], isLoading, error } = useZoneMappings(deviceId);

  // Mutations
  const createMapping = useCreateZoneMapping();
  const updateMapping = useUpdateZoneMapping();
  const deleteMapping = useDeleteZoneMapping();
  const toggleAutoAdjust = useToggleAutoAdjust();
  const setTargetTemp = useSetTargetTemperature();

  // Get unmapped zones
  const mappedZoneIds = new Set(mappings.map((m) => m.tadoZoneId));
  const unmappedZones = availableZones.filter(
    (z) => !mappedZoneIds.has(parseInt(z.roomId, 10))
  );

  const handleAddZone = async (zone: TadoRoom) => {
    await createMapping.mutateAsync({
      deviceId,
      data: {
        tadoZoneId: parseInt(zone.roomId, 10),
        tadoZoneName: zone.roomName,
        targetTemperature: 21.0,
        autoAdjustEnabled: false,
      },
    });
    setShowAddZone(false);
  };

  const handleToggleAutoAdjust = async (mapping: TadoZoneMapping, enabled: boolean) => {
    await toggleAutoAdjust.mutateAsync({
      deviceId,
      mappingId: mapping.id,
      enabled,
      targetTemperature: mapping.targetTemperature,
    });
  };

  const handleTargetChange = async (mapping: TadoZoneMapping, temp: number) => {
    await setTargetTemp.mutateAsync({
      deviceId,
      mappingId: mapping.id,
      targetTemperature: temp,
    });
  };

  const handleDeleteMapping = async (mappingId: string) => {
    if (confirm('Remove this zone mapping? Auto-adjust will be disabled.')) {
      await deleteMapping.mutateAsync({ deviceId, mappingId });
    }
  };

  if (isLoading) {
    return (
      <GlassCard className={cn('p-6', className)}>
        <div className="flex items-center gap-3">
          <div className="h-10 w-10 rounded-xl bg-glass-hover animate-pulse" />
          <div className="flex-1">
            <div className="h-4 w-32 bg-glass-hover rounded animate-pulse mb-2" />
            <div className="h-3 w-48 bg-glass-hover rounded animate-pulse" />
          </div>
        </div>
      </GlassCard>
    );
  }

  if (error) {
    return (
      <GlassCard className={cn('p-6', className)}>
        <div className="flex items-center gap-3 text-error">
          <AlertTriangle className="h-5 w-5" />
          <span className="text-sm">Failed to load zone mappings</span>
        </div>
      </GlassCard>
    );
  }

  return (
    <GlassCard className={cn('overflow-hidden', className)}>
      {/* Header */}
      <button
        className="w-full p-6 flex items-center justify-between hover:bg-glass-hover transition-colors"
        onClick={() => setIsExpanded(!isExpanded)}
      >
        <div className="flex items-center gap-3">
          <div className="h-10 w-10 rounded-xl bg-accent/10 flex items-center justify-center">
            <Gauge className="h-5 w-5 text-accent" />
          </div>
          <div className="text-left">
            <h3 className="text-sm font-medium text-white">Sensor Calibration</h3>
            <p className="text-xs text-text-muted">
              {mappings.length === 0
                ? 'Use ESP32 sensor to calibrate Tado thermostats'
                : mappings.some((m) => m.autoAdjustEnabled)
                ? `${mappings.filter((m) => m.autoAdjustEnabled).length} zone${mappings.filter((m) => m.autoAdjustEnabled).length !== 1 ? 's' : ''} being calibrated`
                : `${mappings.length} zone${mappings.length !== 1 ? 's' : ''} configured`}
            </p>
          </div>
        </div>
        <div className="flex items-center gap-2">
          {mappings.some((m) => m.autoAdjustEnabled) && (
            <Badge variant="primary" size="xs">
              <Zap className="h-3 w-3 mr-1" />
              Active
            </Badge>
          )}
          {isExpanded ? (
            <ChevronUp className="h-5 w-5 text-text-muted" />
          ) : (
            <ChevronDown className="h-5 w-5 text-text-muted" />
          )}
        </div>
      </button>

      {/* Content */}
      <AnimatePresence>
        {isExpanded && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.2 }}
          >
            <div className="px-6 pb-6 space-y-4">
              {/* How it works - collapsible */}
              <div className="rounded-lg bg-accent/5 border border-accent/10 overflow-hidden">
                <button
                  className="w-full p-3 flex items-center justify-between text-left hover:bg-accent/5 transition-colors"
                  onClick={(e) => {
                    e.stopPropagation();
                    setShowHelp(!showHelp);
                  }}
                >
                  <div className="flex items-center gap-2">
                    <Info className="h-4 w-4 text-accent" />
                    <span className="text-sm font-medium text-text-secondary">
                      How does this work?
                    </span>
                  </div>
                  <ChevronDown
                    className={cn(
                      'h-4 w-4 text-text-muted transition-transform',
                      showHelp && 'rotate-180'
                    )}
                  />
                </button>
                <AnimatePresence>
                  {showHelp && (
                    <motion.div
                      initial={{ height: 0, opacity: 0 }}
                      animate={{ height: 'auto', opacity: 1 }}
                      exit={{ height: 0, opacity: 0 }}
                      className="overflow-hidden"
                    >
                      <div className="px-3 pb-3 space-y-3 text-xs text-text-muted">
                        <p>
                          <strong className="text-text-secondary">The Problem:</strong> Tado thermostats
                          measure temperature near the radiator, which is often warmer than the actual room
                          temperature.
                        </p>
                        <p>
                          <strong className="text-text-secondary">The Solution:</strong> This device has a
                          precise sensor placed in the room. When calibration is enabled, it tells Tado
                          how much to adjust its readings.
                        </p>
                        <div className="p-2 rounded bg-glass border border-glass-border">
                          <div className="flex items-center justify-center gap-2 text-text-secondary">
                            <span>ESP32 Sensor</span>
                            <ArrowRight className="h-3 w-3" />
                            <span>Calculates Offset</span>
                            <ArrowRight className="h-3 w-3" />
                            <span>Tado Calibrated</span>
                          </div>
                        </div>
                        <p className="text-text-subtle">
                          Example: If ESP32 reads 22°C but Tado reads 24°C, a -2°C offset is applied
                          so Tado knows the real room temperature.
                        </p>
                      </div>
                    </motion.div>
                  )}
                </AnimatePresence>
              </div>

              {/* Device offline warning */}
              {!isDeviceOnline && (
                <div className="p-3 rounded-lg bg-warning/10 border border-warning/20">
                  <div className="flex items-center gap-2 text-warning text-sm">
                    <AlertTriangle className="h-4 w-4 flex-shrink-0" />
                    <span>Device is offline. Calibration is paused.</span>
                  </div>
                </div>
              )}

              {/* Current sensor reading */}
              {esp32Temp !== undefined && isDeviceOnline && (
                <div className="p-3 rounded-lg bg-glass border border-glass-border">
                  <div className="flex items-center justify-between">
                    <div className="flex items-center gap-2">
                      <Thermometer className="h-4 w-4 text-accent" />
                      <span className="text-sm text-text-secondary">ESP32 Sensor</span>
                    </div>
                    <span className="text-lg font-mono font-bold text-white">
                      {esp32Temp.toFixed(1)}°C
                    </span>
                  </div>
                </div>
              )}

              {/* Mapped zones list */}
              {mappings.length > 0 ? (
                <div className="space-y-3">
                  {mappings.map((mapping) => {
                    // Find the matching Tado zone to get its current temperature
                    const tadoZone = availableZones.find(
                      (z) => parseInt(z.roomId, 10) === mapping.tadoZoneId
                    );
                    return (
                      <ZoneMappingCard
                        key={mapping.id}
                        mapping={mapping}
                        isDeviceOnline={isDeviceOnline}
                        esp32Temp={esp32Temp}
                        tadoTemp={tadoZone?.currentTemp}
                        onToggleAutoAdjust={(enabled) =>
                          handleToggleAutoAdjust(mapping, enabled)
                        }
                        onTargetChange={(temp) => handleTargetChange(mapping, temp)}
                        onDelete={() => handleDeleteMapping(mapping.id)}
                      />
                    );
                  })}
                </div>
              ) : (
                <div className="py-8 text-center">
                  <Thermometer className="h-10 w-10 mx-auto text-text-subtle mb-3" />
                  <p className="text-sm text-text-muted mb-1">No zones configured</p>
                  <p className="text-xs text-text-subtle">
                    Add a Tado zone to enable temperature control with ESP32 sensors
                  </p>
                </div>
              )}

              {/* Add zone section */}
              {unmappedZones.length > 0 && (
                <>
                  {showAddZone ? (
                    <motion.div
                      initial={{ opacity: 0, y: -10 }}
                      animate={{ opacity: 1, y: 0 }}
                      className="p-4 rounded-lg bg-glass border border-glass-border"
                    >
                      <div className="flex items-center justify-between mb-3">
                        <span className="text-sm font-medium text-white">
                          Select a zone to add
                        </span>
                        <Button
                          variant="ghost"
                          size="sm"
                          onClick={() => setShowAddZone(false)}
                        >
                          Cancel
                        </Button>
                      </div>
                      <div className="space-y-2">
                        {unmappedZones.map((zone) => {
                          const potentialOffset =
                            esp32Temp !== undefined
                              ? esp32Temp - zone.currentTemp
                              : null;
                          return (
                            <button
                              key={zone.roomId}
                              className="w-full p-3 rounded-lg bg-glass-hover hover:bg-glass-bright border border-glass-border transition-colors"
                              onClick={() => handleAddZone(zone)}
                              disabled={createMapping.isPending}
                            >
                              <div className="flex items-center justify-between">
                                <div className="flex items-center gap-3">
                                  <Thermometer className="h-4 w-4 text-text-muted" />
                                  <span className="text-sm text-white">{zone.roomName}</span>
                                </div>
                                <div className="flex items-center gap-2 text-xs">
                                  <span className="text-text-muted">
                                    Tado: {zone.currentTemp.toFixed(1)}°C
                                  </span>
                                  {potentialOffset !== null && (
                                    <span
                                      className={cn(
                                        'font-mono px-1.5 py-0.5 rounded',
                                        potentialOffset > 0
                                          ? 'text-orange-400 bg-orange-400/10'
                                          : potentialOffset < 0
                                            ? 'text-blue-400 bg-blue-400/10'
                                            : 'text-text-secondary bg-glass-hover'
                                      )}
                                    >
                                      {potentialOffset > 0 ? '+' : ''}
                                      {potentialOffset.toFixed(1)}°C
                                    </span>
                                  )}
                                </div>
                              </div>
                            </button>
                          );
                        })}
                      </div>
                    </motion.div>
                  ) : (
                    <Button
                      variant="secondary"
                      size="sm"
                      onClick={() => setShowAddZone(true)}
                      className="w-full"
                    >
                      <Plus className="h-4 w-4 mr-2" />
                      Add Zone
                    </Button>
                  )}
                </>
              )}

            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </GlassCard>
  );
}

// ─────────────────────────────────────────────────────────────────────────────
// Zone Mapping Card Component
// ─────────────────────────────────────────────────────────────────────────────

interface ZoneMappingCardProps {
  mapping: TadoZoneMapping;
  isDeviceOnline?: boolean;
  esp32Temp?: number;
  tadoTemp?: number;
  onToggleAutoAdjust: (enabled: boolean) => void;
  onTargetChange: (temp: number) => void;
  onDelete: () => void;
}

function ZoneMappingCard({
  mapping,
  isDeviceOnline = true,
  esp32Temp,
  tadoTemp,
  onToggleAutoAdjust,
  onTargetChange,
  onDelete,
}: ZoneMappingCardProps) {
  const [isEditing, setIsEditing] = useState(false);
  const [tempValue, setTempValue] = useState(mapping.targetTemperature);

  // Calculate current offset
  const currentOffset =
    esp32Temp !== undefined && tadoTemp !== undefined ? esp32Temp - tadoTemp : null;

  const handleTempChange = (value: number) => {
    setTempValue(value);
  };

  const handleTempCommit = () => {
    if (tempValue !== mapping.targetTemperature) {
      onTargetChange(tempValue);
    }
  };

  return (
    <div
      className={cn(
        'p-4 rounded-lg border transition-all',
        mapping.autoAdjustEnabled && isDeviceOnline
          ? 'bg-accent/5 border-accent/20'
          : 'bg-glass border-glass-border'
      )}
    >
      {/* Header row with zone name and controls */}
      <div className="flex items-center justify-between mb-3">
        <div className="flex items-center gap-2">
          <div
            className={cn(
              'h-8 w-8 rounded-lg flex items-center justify-center',
              mapping.autoAdjustEnabled && isDeviceOnline
                ? 'bg-accent/20'
                : 'bg-glass-hover'
            )}
          >
            {mapping.autoAdjustEnabled && isDeviceOnline ? (
              <Zap className="h-4 w-4 text-accent" />
            ) : (
              <Thermometer className="h-4 w-4 text-text-subtle" />
            )}
          </div>
          <div>
            <span className="text-sm font-medium text-white block">
              {mapping.tadoZoneName}
            </span>
            {!mapping.autoAdjustEnabled && (
              <span className="text-xs text-text-muted">Calibration disabled</span>
            )}
          </div>
        </div>

        <div className="flex items-center gap-2">
          <div className="flex items-center gap-1.5 mr-1">
            <span className="text-xs text-text-muted">Calibrate</span>
            <Toggle
              size="sm"
              checked={mapping.autoAdjustEnabled}
              onCheckedChange={onToggleAutoAdjust}
              disabled={!isDeviceOnline}
            />
          </div>
          <Button
            variant="ghost"
            size="icon-sm"
            onClick={() => setIsEditing(!isEditing)}
            aria-label="Edit zone"
          >
            <Settings className="h-4 w-4" />
          </Button>
        </div>
      </div>

      {/* Offset display - only when calibration is active and we have both temps */}
      {mapping.autoAdjustEnabled && isDeviceOnline && currentOffset !== null && (
        <div className="mb-3 p-3 rounded-lg bg-glass border border-glass-border">
          <div className="grid grid-cols-3 gap-2 text-center">
            {/* ESP32 reading */}
            <div>
              <div className="text-xs text-text-muted mb-1">ESP32</div>
              <div className="text-sm font-mono font-semibold text-accent">
                {esp32Temp!.toFixed(1)}°C
              </div>
            </div>

            {/* Offset indicator */}
            <div className="flex flex-col items-center justify-center">
              <div className="text-xs text-text-muted mb-1">Offset</div>
              <div
                className={cn(
                  'text-sm font-mono font-bold px-2 py-0.5 rounded',
                  currentOffset > 0
                    ? 'text-orange-400 bg-orange-400/10'
                    : currentOffset < 0
                      ? 'text-blue-400 bg-blue-400/10'
                      : 'text-text-secondary bg-glass-hover'
                )}
              >
                {currentOffset > 0 ? '+' : ''}
                {currentOffset.toFixed(1)}°C
              </div>
            </div>

            {/* Tado reading */}
            <div>
              <div className="text-xs text-text-muted mb-1">Tado</div>
              <div className="text-sm font-mono font-semibold text-text-secondary">
                {tadoTemp!.toFixed(1)}°C
              </div>
            </div>
          </div>
          <p className="text-xs text-text-subtle text-center mt-2">
            {currentOffset > 0
              ? 'Room is warmer than Tado reads'
              : currentOffset < 0
                ? 'Room is cooler than Tado reads'
                : 'Tado is reading accurately'}
          </p>
        </div>
      )}

      {/* Expandable edit section */}
      <AnimatePresence>
        {isEditing && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            className="overflow-hidden"
          >
            <div className="pt-3 border-t border-glass-border space-y-4">
              {/* Target temperature slider */}
              <div>
                <div className="flex items-center justify-between mb-2">
                  <span className="text-xs text-text-muted">Target Temperature</span>
                  <span className="text-sm font-mono text-white">
                    {tempValue.toFixed(1)}°C
                  </span>
                </div>
                <Slider
                  value={tempValue}
                  min={5}
                  max={25}
                  step={0.5}
                  onChange={handleTempChange}
                  onChangeEnd={handleTempCommit}
                  disabled={!isDeviceOnline}
                />
                <div className="flex justify-between text-xs text-text-subtle mt-1">
                  <span>5°C</span>
                  <span>25°C</span>
                </div>
              </div>

              {/* Delete button */}
              <Button
                variant="ghost"
                size="sm"
                onClick={onDelete}
                className="w-full text-error hover:bg-error/10 hover:text-error"
              >
                <Trash2 className="h-4 w-4 mr-2" />
                Remove Zone
              </Button>
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

export default ZoneMappingSection;
