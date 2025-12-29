import { Variants, Transition } from 'framer-motion';

/**
 * PaperHome Animation System
 * Smooth, subtle animations for a premium feel
 */

// ─────────────────────────────────────────────────────────────────────────────
// Transition Presets
// ─────────────────────────────────────────────────────────────────────────────

export const transitions = {
  /** Fast transition for micro-interactions */
  fast: {
    duration: 0.15,
    ease: [0.4, 0, 0.2, 1],
  } as Transition,

  /** Default transition for most animations */
  default: {
    duration: 0.2,
    ease: [0.4, 0, 0.2, 1],
  } as Transition,

  /** Smooth transition for larger movements */
  smooth: {
    duration: 0.3,
    ease: [0.4, 0, 0.2, 1],
  } as Transition,

  /** Slow transition for emphasis */
  slow: {
    duration: 0.5,
    ease: [0.4, 0, 0.2, 1],
  } as Transition,

  /** Spring animation for playful interactions */
  spring: {
    type: 'spring',
    stiffness: 400,
    damping: 30,
  } as Transition,

  /** Gentle spring for subtle movements */
  gentleSpring: {
    type: 'spring',
    stiffness: 200,
    damping: 25,
  } as Transition,

  /** Bouncy spring for attention-grabbing */
  bounce: {
    type: 'spring',
    stiffness: 500,
    damping: 15,
  } as Transition,
};

// ─────────────────────────────────────────────────────────────────────────────
// Page Transitions
// ─────────────────────────────────────────────────────────────────────────────

export const pageTransition: Variants = {
  initial: {
    opacity: 0,
    y: 20,
  },
  animate: {
    opacity: 1,
    y: 0,
    transition: transitions.smooth,
  },
  exit: {
    opacity: 0,
    y: -20,
    transition: transitions.default,
  },
};

export const pageSlideRight: Variants = {
  initial: {
    opacity: 0,
    x: 40,
  },
  animate: {
    opacity: 1,
    x: 0,
    transition: transitions.smooth,
  },
  exit: {
    opacity: 0,
    x: -40,
    transition: transitions.default,
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Container Animations (for staggering children)
// ─────────────────────────────────────────────────────────────────────────────

export const staggerContainer: Variants = {
  initial: {},
  animate: {
    transition: {
      staggerChildren: 0.05,
      delayChildren: 0.1,
    },
  },
  exit: {
    transition: {
      staggerChildren: 0.03,
      staggerDirection: -1,
    },
  },
};

export const staggerContainerFast: Variants = {
  initial: {},
  animate: {
    transition: {
      staggerChildren: 0.03,
      delayChildren: 0.05,
    },
  },
};

export const staggerContainerSlow: Variants = {
  initial: {},
  animate: {
    transition: {
      staggerChildren: 0.1,
      delayChildren: 0.2,
    },
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Item Animations (children of stagger containers)
// ─────────────────────────────────────────────────────────────────────────────

export const fadeInUp: Variants = {
  initial: {
    opacity: 0,
    y: 20,
  },
  animate: {
    opacity: 1,
    y: 0,
    transition: transitions.smooth,
  },
  exit: {
    opacity: 0,
    y: 10,
    transition: transitions.fast,
  },
};

export const fadeIn: Variants = {
  initial: {
    opacity: 0,
  },
  animate: {
    opacity: 1,
    transition: transitions.default,
  },
  exit: {
    opacity: 0,
    transition: transitions.fast,
  },
};

export const scaleIn: Variants = {
  initial: {
    opacity: 0,
    scale: 0.95,
  },
  animate: {
    opacity: 1,
    scale: 1,
    transition: transitions.spring,
  },
  exit: {
    opacity: 0,
    scale: 0.95,
    transition: transitions.fast,
  },
};

export const slideInLeft: Variants = {
  initial: {
    opacity: 0,
    x: -20,
  },
  animate: {
    opacity: 1,
    x: 0,
    transition: transitions.smooth,
  },
  exit: {
    opacity: 0,
    x: -10,
    transition: transitions.fast,
  },
};

export const slideInRight: Variants = {
  initial: {
    opacity: 0,
    x: 20,
  },
  animate: {
    opacity: 1,
    x: 0,
    transition: transitions.smooth,
  },
  exit: {
    opacity: 0,
    x: 10,
    transition: transitions.fast,
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Card Animations
// ─────────────────────────────────────────────────────────────────────────────

export const cardHover = {
  rest: {
    y: 0,
    scale: 1,
    boxShadow: '0 4px 12px rgba(0, 0, 0, 0.4)',
  },
  hover: {
    y: -4,
    scale: 1.02,
    boxShadow: '0 8px 24px rgba(0, 0, 0, 0.5)',
    transition: transitions.spring,
  },
  tap: {
    scale: 0.98,
    transition: transitions.fast,
  },
};

export const glowHover = {
  rest: {
    boxShadow: '0 0 30px hsla(187, 100%, 50%, 0.15)',
  },
  hover: {
    boxShadow: '0 0 50px hsla(187, 100%, 50%, 0.25)',
    transition: transitions.smooth,
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Modal & Overlay Animations
// ─────────────────────────────────────────────────────────────────────────────

export const overlayAnimation: Variants = {
  initial: {
    opacity: 0,
  },
  animate: {
    opacity: 1,
    transition: transitions.default,
  },
  exit: {
    opacity: 0,
    transition: transitions.fast,
  },
};

export const modalAnimation: Variants = {
  initial: {
    opacity: 0,
    scale: 0.95,
    y: 10,
  },
  animate: {
    opacity: 1,
    scale: 1,
    y: 0,
    transition: transitions.spring,
  },
  exit: {
    opacity: 0,
    scale: 0.95,
    y: 10,
    transition: transitions.fast,
  },
};

export const slideUpModal: Variants = {
  initial: {
    opacity: 0,
    y: '100%',
  },
  animate: {
    opacity: 1,
    y: 0,
    transition: {
      type: 'spring',
      stiffness: 300,
      damping: 30,
    },
  },
  exit: {
    opacity: 0,
    y: '100%',
    transition: transitions.smooth,
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Dropdown & Popover Animations
// ─────────────────────────────────────────────────────────────────────────────

export const dropdownAnimation: Variants = {
  initial: {
    opacity: 0,
    y: -10,
    scale: 0.95,
  },
  animate: {
    opacity: 1,
    y: 0,
    scale: 1,
    transition: {
      duration: 0.15,
      ease: [0.4, 0, 0.2, 1],
    },
  },
  exit: {
    opacity: 0,
    y: -10,
    scale: 0.95,
    transition: {
      duration: 0.1,
      ease: [0.4, 0, 1, 1],
    },
  },
};

export const popoverAnimation: Variants = {
  initial: {
    opacity: 0,
    scale: 0.9,
  },
  animate: {
    opacity: 1,
    scale: 1,
    transition: transitions.spring,
  },
  exit: {
    opacity: 0,
    scale: 0.9,
    transition: transitions.fast,
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Notification Animations
// ─────────────────────────────────────────────────────────────────────────────

export const toastAnimation: Variants = {
  initial: {
    opacity: 0,
    y: -20,
    scale: 0.9,
  },
  animate: {
    opacity: 1,
    y: 0,
    scale: 1,
    transition: transitions.spring,
  },
  exit: {
    opacity: 0,
    y: -20,
    scale: 0.9,
    transition: transitions.default,
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Interactive Element Animations
// ─────────────────────────────────────────────────────────────────────────────

export const buttonTap = {
  tap: {
    scale: 0.97,
    transition: transitions.fast,
  },
};

export const iconSpin = {
  animate: {
    rotate: 360,
    transition: {
      duration: 1,
      repeat: Infinity,
      ease: 'linear',
    },
  },
};

export const pulse = {
  animate: {
    scale: [1, 1.05, 1],
    opacity: [1, 0.8, 1],
    transition: {
      duration: 2,
      repeat: Infinity,
      ease: 'easeInOut',
    },
  },
};

// ─────────────────────────────────────────────────────────────────────────────
// Chart & Data Animations
// ─────────────────────────────────────────────────────────────────────────────

export const drawLine = {
  initial: {
    pathLength: 0,
    opacity: 0,
  },
  animate: {
    pathLength: 1,
    opacity: 1,
    transition: {
      pathLength: { duration: 1, ease: 'easeInOut' },
      opacity: { duration: 0.3 },
    },
  },
};

export const countUp = (from: number, to: number, duration = 1) => ({
  initial: { value: from },
  animate: {
    value: to,
    transition: { duration, ease: 'easeOut' },
  },
});

// ─────────────────────────────────────────────────────────────────────────────
// Gauge/Arc Animations
// ─────────────────────────────────────────────────────────────────────────────

export const gaugeArc = (startAngle: number, endAngle: number) => ({
  initial: {
    pathLength: 0,
  },
  animate: {
    pathLength: 1,
    transition: {
      duration: 1.2,
      ease: [0.4, 0, 0.2, 1],
    },
  },
});

// ─────────────────────────────────────────────────────────────────────────────
// Utility function to create staggered delay
// ─────────────────────────────────────────────────────────────────────────────

export const getStaggerDelay = (index: number, baseDelay = 0.05) => ({
  transition: {
    delay: index * baseDelay,
  },
});
