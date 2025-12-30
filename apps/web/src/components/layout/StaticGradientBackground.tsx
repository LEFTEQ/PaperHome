import { memo } from 'react';

/**
 * Static gradient background - performant alternative to animated blobs.
 * Creates a subtle, atmospheric backdrop with fixed gradient overlays.
 */
export const StaticGradientBackground = memo(function StaticGradientBackground() {
  return (
    <>
      {/* Static gradient background */}
      <div className="static-gradient-bg" />

      {/* Subtle noise texture overlay */}
      <div className="noise-overlay" />
    </>
  );
});

export default StaticGradientBackground;
