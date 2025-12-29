/**
 * MQTT Topic Constants for PaperHome
 *
 * Pattern: paperhome/{deviceId}/{topic}
 */

export const MQTT_TOPIC_PREFIX = 'paperhome';

/**
 * Build MQTT topic with device ID
 */
export function buildTopic(deviceId: string, topic: string): string {
  return `${MQTT_TOPIC_PREFIX}/${deviceId}/${topic}`;
}

/**
 * Topics ESP32 publishes TO server
 */
export const ESP32_PUBLISH_TOPICS = {
  /** CO2, temp, humidity, battery readings */
  TELEMETRY: 'telemetry',
  /** Online/offline heartbeat */
  STATUS: 'status',
  /** Hue room states */
  HUE_STATE: 'hue/state',
  /** Tado room states */
  TADO_STATE: 'tado/state',
  /** Command acknowledgment */
  COMMAND_ACK: 'command/ack',
} as const;

/**
 * Topics server publishes TO ESP32
 */
export const SERVER_PUBLISH_TOPICS = {
  /** Commands for device to execute */
  COMMAND: 'command',
} as const;

/**
 * Topic patterns for subscription (wildcards)
 */
export const SUBSCRIPTION_PATTERNS = {
  /** All telemetry from all devices */
  ALL_TELEMETRY: `${MQTT_TOPIC_PREFIX}/+/telemetry`,
  /** All status from all devices */
  ALL_STATUS: `${MQTT_TOPIC_PREFIX}/+/status`,
  /** All Hue state from all devices */
  ALL_HUE_STATE: `${MQTT_TOPIC_PREFIX}/+/hue/state`,
  /** All Tado state from all devices */
  ALL_TADO_STATE: `${MQTT_TOPIC_PREFIX}/+/tado/state`,
  /** All command acks from all devices */
  ALL_COMMAND_ACK: `${MQTT_TOPIC_PREFIX}/+/command/ack`,
} as const;

/**
 * Extract device ID from topic
 */
export function extractDeviceId(topic: string): string | null {
  const parts = topic.split('/');
  if (parts[0] === MQTT_TOPIC_PREFIX && parts.length >= 2) {
    return parts[1];
  }
  return null;
}

/**
 * Extract message type from topic
 */
export function extractMessageType(topic: string): string | null {
  const parts = topic.split('/');
  if (parts[0] === MQTT_TOPIC_PREFIX && parts.length >= 3) {
    return parts.slice(2).join('/');
  }
  return null;
}
