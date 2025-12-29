import { memo } from 'react';
import { motion } from 'framer-motion';

/**
 * AnimatedBackground
 *
 * Creates an atmospheric dark background with animated gradient blobs
 * and a subtle noise texture overlay for depth.
 */
export const AnimatedBackground = memo(function AnimatedBackground() {
  return (
    <>
      {/* Base gradient background */}
      <div
        className="fixed inset-0 -z-20"
        style={{
          background: `
            radial-gradient(ellipse 80% 50% at 50% -20%, hsla(187, 100%, 50%, 0.08), transparent),
            radial-gradient(ellipse 60% 40% at 100% 100%, hsla(160, 84%, 45%, 0.05), transparent),
            hsl(228, 15%, 4%)
          `,
        }}
      />

      {/* Animated gradient blobs */}
      <div className="fixed inset-0 -z-10 overflow-hidden pointer-events-none">
        {/* Primary cyan blob - top left */}
        <motion.div
          className="absolute w-[600px] h-[600px] rounded-full"
          style={{
            top: '-200px',
            left: '-100px',
            background: 'radial-gradient(circle, hsla(187, 100%, 50%, 0.12) 0%, transparent 70%)',
            filter: 'blur(80px)',
          }}
          animate={{
            x: [0, 30, -20, 0],
            y: [0, -30, 20, 0],
            scale: [1, 1.05, 0.95, 1],
          }}
          transition={{
            duration: 20,
            repeat: Infinity,
            ease: 'easeInOut',
          }}
        />

        {/* Secondary emerald blob - bottom right */}
        <motion.div
          className="absolute w-[500px] h-[500px] rounded-full"
          style={{
            bottom: '-150px',
            right: '-100px',
            background: 'radial-gradient(circle, hsla(160, 84%, 45%, 0.08) 0%, transparent 70%)',
            filter: 'blur(80px)',
          }}
          animate={{
            x: [0, -20, 30, 0],
            y: [0, 20, -30, 0],
            scale: [1, 0.95, 1.05, 1],
          }}
          transition={{
            duration: 25,
            repeat: Infinity,
            ease: 'easeInOut',
            delay: 2,
          }}
        />

        {/* Tertiary purple blob - center */}
        <motion.div
          className="absolute w-[400px] h-[400px] rounded-full"
          style={{
            top: '50%',
            left: '50%',
            transform: 'translate(-50%, -50%)',
            background: 'radial-gradient(circle, hsla(280, 70%, 50%, 0.06) 0%, transparent 70%)',
            filter: 'blur(80px)',
          }}
          animate={{
            x: [0, -30, 30, 0],
            y: [0, 30, -30, 0],
            scale: [1, 1.1, 0.9, 1],
          }}
          transition={{
            duration: 30,
            repeat: Infinity,
            ease: 'easeInOut',
            delay: 5,
          }}
        />

        {/* Accent warm blob - mid right */}
        <motion.div
          className="absolute w-[350px] h-[350px] rounded-full opacity-60"
          style={{
            top: '30%',
            right: '10%',
            background: 'radial-gradient(circle, hsla(38, 92%, 50%, 0.04) 0%, transparent 70%)',
            filter: 'blur(60px)',
          }}
          animate={{
            x: [0, 20, -20, 0],
            y: [0, -20, 20, 0],
            scale: [1, 1.08, 0.92, 1],
          }}
          transition={{
            duration: 22,
            repeat: Infinity,
            ease: 'easeInOut',
            delay: 8,
          }}
        />
      </div>

      {/* Noise texture overlay */}
      <div
        className="fixed inset-0 z-[9998] pointer-events-none opacity-[0.015]"
        style={{
          backgroundImage: `url("data:image/svg+xml,%3Csvg viewBox='0 0 256 256' xmlns='http://www.w3.org/2000/svg'%3E%3Cfilter id='noise'%3E%3CfeTurbulence type='fractalNoise' baseFrequency='0.9' numOctaves='4' stitchTiles='stitch'/%3E%3C/filter%3E%3Crect width='100%25' height='100%25' filter='url(%23noise)'/%3E%3C/svg%3E")`,
        }}
      />

      {/* Subtle vignette effect */}
      <div
        className="fixed inset-0 -z-5 pointer-events-none"
        style={{
          background: 'radial-gradient(ellipse at center, transparent 0%, hsla(228, 15%, 4%, 0.4) 100%)',
        }}
      />
    </>
  );
});

/**
 * SimpleBackground
 *
 * A lighter-weight version without animations for performance-sensitive contexts.
 */
export const SimpleBackground = memo(function SimpleBackground() {
  return (
    <div
      className="fixed inset-0 -z-20"
      style={{
        background: `
          radial-gradient(ellipse 80% 50% at 50% -20%, hsla(187, 100%, 50%, 0.08), transparent),
          radial-gradient(ellipse 60% 40% at 100% 100%, hsla(160, 84%, 45%, 0.05), transparent),
          hsl(228, 15%, 4%)
        `,
      }}
    />
  );
});

export default AnimatedBackground;
