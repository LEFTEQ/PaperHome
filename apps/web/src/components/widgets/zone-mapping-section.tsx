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
  className,
}: ZoneMappingSectionProps) {
  const [isExpanded, setIsExpanded] = useState(true);
  const [showAddZone, setShowAddZone] = useState(false);

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
            <Settings className="h-5 w-5 text-accent" />
          </div>
          <div className="text-left">
            <h3 className="text-sm font-medium text-white">Zone Mapping</h3>
            <p className="text-xs text-text-muted">
              {mappings.length === 0
                ? 'Link Tado zones to use ESP32 sensors'
                : `${mappings.length} zone${mappings.length !== 1 ? 's' : ''} configured`}
            </p>
          </div>
        </div>
        <div className="flex items-center gap-2">
          {mappings.some((m) => m.autoAdjustEnabled) && (
            <Badge variant="accent" size="xs">
              <Zap className="h-3 w-3 mr-1" />
              Auto-Adjust Active
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
              {/* Device offline warning */}
              {!isDeviceOnline && (
                <div className="p-3 rounded-lg bg-warning/10 border border-warning/20">
                  <div className="flex items-center gap-2 text-warning text-sm">
                    <AlertTriangle className="h-4 w-4 flex-shrink-0" />
                    <span>Device is offline. Auto-adjust is paused.</span>
                  </div>
                </div>
              )}

              {/* Mapped zones list */}
              {mappings.length > 0 ? (
                <div className="space-y-3">
                  {mappings.map((mapping) => (
                    <ZoneMappingCard
                      key={mapping.id}
                      mapping={mapping}
                      isDeviceOnline={isDeviceOnline}
                      onToggleAutoAdjust={(enabled) =>
                        handleToggleAutoAdjust(mapping, enabled)
                      }
                      onTargetChange={(temp) => handleTargetChange(mapping, temp)}
                      onDelete={() => handleDeleteMapping(mapping.id)}
                    />
                  ))}
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
                        {unmappedZones.map((zone) => (
                          <button
                            key={zone.id}
                            className="w-full p-3 rounded-lg bg-glass-hover hover:bg-glass-bright border border-glass-border transition-colors flex items-center justify-between"
                            onClick={() => handleAddZone(zone)}
                            disabled={createMapping.isPending}
                          >
                            <div className="flex items-center gap-3">
                              <Thermometer className="h-4 w-4 text-text-muted" />
                              <span className="text-sm text-white">{zone.roomName}</span>
                            </div>
                            <span className="text-xs text-text-muted">
                              {zone.currentTemp.toFixed(1)}°C
                            </span>
                          </button>
                        ))}
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

              {/* Info about auto-adjust */}
              <div className="p-3 rounded-lg bg-accent/5 border border-accent/10">
                <p className="text-xs text-text-muted">
                  <strong className="text-text-secondary">Auto-Adjust:</strong> When enabled,
                  {deviceName || 'this device'}'s ESP32 sensor will control Tado thermostats
                  instead of Tado's built-in sensors, providing more accurate room temperature
                  control.
                </p>
              </div>
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
  onToggleAutoAdjust: (enabled: boolean) => void;
  onTargetChange: (temp: number) => void;
  onDelete: () => void;
}

function ZoneMappingCard({
  mapping,
  isDeviceOnline = true,
  onToggleAutoAdjust,
  onTargetChange,
  onDelete,
}: ZoneMappingCardProps) {
  const [isEditing, setIsEditing] = useState(false);
  const [tempValue, setTempValue] = useState(mapping.targetTemperature);

  const handleTempChange = (value: number[]) => {
    setTempValue(value[0]);
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
            <span className="text-xs text-text-muted">
              Target: {mapping.targetTemperature.toFixed(1)}°C
            </span>
          </div>
        </div>

        <div className="flex items-center gap-2">
          <Toggle
            size="sm"
            checked={mapping.autoAdjustEnabled}
            onCheckedChange={onToggleAutoAdjust}
            disabled={!isDeviceOnline}
          />
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
                  value={[tempValue]}
                  min={5}
                  max={25}
                  step={0.5}
                  onValueChange={handleTempChange}
                  onValueCommit={handleTempCommit}
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
