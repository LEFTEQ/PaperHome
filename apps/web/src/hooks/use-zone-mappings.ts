import { useQuery, useMutation, useQueryClient } from '@tanstack/react-query';
import { tadoApi, TadoZoneMapping, CreateZoneMappingDto, UpdateZoneMappingDto } from '@/lib/api';

/**
 * Query key factory for zone mappings
 */
export const zoneMappingKeys = {
  all: ['zone-mappings'] as const,
  byDevice: (deviceId: string) => ['zone-mappings', deviceId] as const,
};

/**
 * Fetch zone mappings for a device
 */
export function useZoneMappings(deviceId: string | undefined) {
  return useQuery({
    queryKey: zoneMappingKeys.byDevice(deviceId!),
    queryFn: () => tadoApi.getZoneMappings(deviceId!),
    enabled: !!deviceId,
    staleTime: 30 * 1000, // 30 seconds
  });
}

/**
 * Create a new zone mapping
 */
export function useCreateZoneMapping() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({ deviceId, data }: { deviceId: string; data: CreateZoneMappingDto }) =>
      tadoApi.createZoneMapping(deviceId, data),
    onSuccess: (_, { deviceId }) => {
      // Invalidate zone mappings cache
      queryClient.invalidateQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });
    },
  });
}

/**
 * Update an existing zone mapping
 */
export function useUpdateZoneMapping() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({
      deviceId,
      mappingId,
      data,
    }: {
      deviceId: string;
      mappingId: string;
      data: UpdateZoneMappingDto;
    }) => tadoApi.updateZoneMapping(deviceId, mappingId, data),
    onMutate: async ({ deviceId, mappingId, data }) => {
      // Cancel outgoing refetches
      await queryClient.cancelQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });

      // Snapshot previous value
      const previousMappings = queryClient.getQueryData<TadoZoneMapping[]>(
        zoneMappingKeys.byDevice(deviceId)
      );

      // Optimistically update cache
      if (previousMappings) {
        queryClient.setQueryData<TadoZoneMapping[]>(
          zoneMappingKeys.byDevice(deviceId),
          previousMappings.map((m) =>
            m.id === mappingId ? { ...m, ...data, updatedAt: new Date().toISOString() } : m
          )
        );
      }

      return { previousMappings };
    },
    onError: (_, { deviceId }, context) => {
      // Rollback on error
      if (context?.previousMappings) {
        queryClient.setQueryData(zoneMappingKeys.byDevice(deviceId), context.previousMappings);
      }
    },
    onSettled: (_, __, { deviceId }) => {
      // Refetch after mutation
      queryClient.invalidateQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });
    },
  });
}

/**
 * Delete a zone mapping
 */
export function useDeleteZoneMapping() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({ deviceId, mappingId }: { deviceId: string; mappingId: string }) =>
      tadoApi.deleteZoneMapping(deviceId, mappingId),
    onMutate: async ({ deviceId, mappingId }) => {
      // Cancel outgoing refetches
      await queryClient.cancelQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });

      // Snapshot previous value
      const previousMappings = queryClient.getQueryData<TadoZoneMapping[]>(
        zoneMappingKeys.byDevice(deviceId)
      );

      // Optimistically remove from cache
      if (previousMappings) {
        queryClient.setQueryData<TadoZoneMapping[]>(
          zoneMappingKeys.byDevice(deviceId),
          previousMappings.filter((m) => m.id !== mappingId)
        );
      }

      return { previousMappings };
    },
    onError: (_, { deviceId }, context) => {
      // Rollback on error
      if (context?.previousMappings) {
        queryClient.setQueryData(zoneMappingKeys.byDevice(deviceId), context.previousMappings);
      }
    },
    onSettled: (_, __, { deviceId }) => {
      // Refetch after mutation
      queryClient.invalidateQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });
    },
  });
}

/**
 * Toggle auto-adjust for a zone mapping with optimistic update
 */
export function useToggleAutoAdjust() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({
      deviceId,
      mappingId,
      enabled,
      targetTemperature,
    }: {
      deviceId: string;
      mappingId: string;
      enabled: boolean;
      targetTemperature?: number;
    }) => tadoApi.setAutoAdjust(deviceId, mappingId, enabled, targetTemperature),
    onMutate: async ({ deviceId, mappingId, enabled, targetTemperature }) => {
      // Cancel outgoing refetches
      await queryClient.cancelQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });

      // Snapshot previous value
      const previousMappings = queryClient.getQueryData<TadoZoneMapping[]>(
        zoneMappingKeys.byDevice(deviceId)
      );

      // Optimistically update cache
      if (previousMappings) {
        queryClient.setQueryData<TadoZoneMapping[]>(
          zoneMappingKeys.byDevice(deviceId),
          previousMappings.map((m) =>
            m.id === mappingId
              ? {
                  ...m,
                  autoAdjustEnabled: enabled,
                  ...(targetTemperature !== undefined && { targetTemperature }),
                  updatedAt: new Date().toISOString(),
                }
              : m
          )
        );
      }

      return { previousMappings };
    },
    onError: (_, { deviceId }, context) => {
      // Rollback on error
      if (context?.previousMappings) {
        queryClient.setQueryData(zoneMappingKeys.byDevice(deviceId), context.previousMappings);
      }
    },
    onSettled: (_, __, { deviceId }) => {
      // Refetch after mutation
      queryClient.invalidateQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });
    },
  });
}

/**
 * Set target temperature for a zone mapping with optimistic update
 */
export function useSetTargetTemperature() {
  const queryClient = useQueryClient();

  return useMutation({
    mutationFn: ({
      deviceId,
      mappingId,
      targetTemperature,
    }: {
      deviceId: string;
      mappingId: string;
      targetTemperature: number;
    }) => tadoApi.updateZoneMapping(deviceId, mappingId, { targetTemperature }),
    onMutate: async ({ deviceId, mappingId, targetTemperature }) => {
      // Cancel outgoing refetches
      await queryClient.cancelQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });

      // Snapshot previous value
      const previousMappings = queryClient.getQueryData<TadoZoneMapping[]>(
        zoneMappingKeys.byDevice(deviceId)
      );

      // Optimistically update cache
      if (previousMappings) {
        queryClient.setQueryData<TadoZoneMapping[]>(
          zoneMappingKeys.byDevice(deviceId),
          previousMappings.map((m) =>
            m.id === mappingId
              ? { ...m, targetTemperature, updatedAt: new Date().toISOString() }
              : m
          )
        );
      }

      return { previousMappings };
    },
    onError: (_, { deviceId }, context) => {
      // Rollback on error
      if (context?.previousMappings) {
        queryClient.setQueryData(zoneMappingKeys.byDevice(deviceId), context.previousMappings);
      }
    },
    onSettled: (_, __, { deviceId }) => {
      // Refetch after mutation
      queryClient.invalidateQueries({ queryKey: zoneMappingKeys.byDevice(deviceId) });
    },
  });
}
