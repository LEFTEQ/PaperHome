import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { Sparkles, Loader2, Check, X } from 'lucide-react';
import { cn } from '@/lib/utils';

export interface ClaimDeviceBannerProps {
  deviceName: string;
  deviceId: string;
  onClaim: () => Promise<void>;
  onDismiss?: () => void;
  className?: string;
}

export function ClaimDeviceBanner({
  deviceName,
  deviceId,
  onClaim,
  onDismiss,
  className,
}: ClaimDeviceBannerProps) {
  const [isLoading, setIsLoading] = useState(false);
  const [isSuccess, setIsSuccess] = useState(false);
  const [error, setError] = useState<string | null>(null);

  const handleClaim = async () => {
    setIsLoading(true);
    setError(null);

    try {
      await onClaim();
      setIsSuccess(true);
    } catch (err) {
      setError('Failed to claim device. Please try again.');
    } finally {
      setIsLoading(false);
    }
  };

  if (isSuccess) {
    return (
      <motion.div
        className={cn('claim-banner', className)}
        initial={{ opacity: 0, y: -10 }}
        animate={{ opacity: 1, y: 0 }}
        exit={{ opacity: 0, y: -10 }}
      >
        <div className="flex items-center gap-3">
          <div className="w-10 h-10 rounded-xl bg-success/20 flex items-center justify-center">
            <Check className="w-5 h-5 text-success" />
          </div>
          <div>
            <p className="font-medium text-white">Device claimed!</p>
            <p className="text-sm text-text-muted">
              {deviceName} is now linked to your account.
            </p>
          </div>
        </div>
      </motion.div>
    );
  }

  return (
    <motion.div
      className={cn('claim-banner', className)}
      initial={{ opacity: 0, y: -10 }}
      animate={{ opacity: 1, y: 0 }}
      exit={{ opacity: 0, y: -10 }}
    >
      <div className="flex items-center gap-3">
        <div className="w-10 h-10 rounded-xl bg-accent/20 flex items-center justify-center">
          <Sparkles className="w-5 h-5 text-accent" />
        </div>
        <div>
          <p className="font-medium text-white">
            New device detected: {deviceName}
          </p>
          <p className="text-sm text-text-muted">
            Claim this device to add it to your account and enable full control.
          </p>
          {error && <p className="text-sm text-error mt-1">{error}</p>}
        </div>
      </div>

      <div className="flex items-center gap-2">
        {onDismiss && (
          <button
            onClick={onDismiss}
            className="p-2 rounded-lg hover:bg-glass transition-colors"
            disabled={isLoading}
          >
            <X className="w-5 h-5 text-text-muted" />
          </button>
        )}
        <button
          onClick={handleClaim}
          disabled={isLoading}
          className={cn(
            'claim-button flex items-center gap-2',
            isLoading && 'opacity-70 cursor-not-allowed'
          )}
        >
          {isLoading ? (
            <>
              <Loader2 className="w-4 h-4 animate-spin" />
              Claiming...
            </>
          ) : (
            'Claim Device'
          )}
        </button>
      </div>
    </motion.div>
  );
}

export default ClaimDeviceBanner;
