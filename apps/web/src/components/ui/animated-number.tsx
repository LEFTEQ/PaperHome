import { useEffect, useRef } from 'react';
import { motion, useSpring, useTransform } from 'framer-motion';
import { cn } from '@/lib/utils';

interface AnimatedNumberProps {
  value: number;
  decimals?: number;
  duration?: number;
  className?: string;
}

export function AnimatedNumber({
  value,
  decimals = 0,
  duration = 0.5,
  className,
}: AnimatedNumberProps) {
  const spring = useSpring(0, {
    mass: 0.8,
    stiffness: 75,
    damping: 15,
    duration: duration * 1000,
  });

  const display = useTransform(spring, (current) =>
    new Intl.NumberFormat(undefined, {
      minimumFractionDigits: decimals,
      maximumFractionDigits: decimals,
    }).format(current)
  );

  const prevValue = useRef(value);

  useEffect(() => {
    // Only animate if value actually changed
    if (prevValue.current !== value) {
      spring.set(value);
      prevValue.current = value;
    }
  }, [value, spring]);

  // Initialize spring with current value on mount (no animation)
  useEffect(() => {
    spring.jump(value);
  }, []);

  return <motion.span className={cn('tabular-nums', className)}>{display}</motion.span>;
}
