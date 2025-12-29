import { useState } from 'react';
import { motion } from 'framer-motion';
import { useAuth } from '@/context/auth-context';
import { GlassCard } from '@/components/ui/glass-card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Badge } from '@/components/ui/badge';
import { Toggle } from '@/components/ui/toggle';
import { staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';
import { User, Key, Shield, Bell, Save, CheckCircle } from 'lucide-react';

export function SettingsPage() {
  const { user } = useAuth();
  const [displayName, setDisplayName] = useState(user?.displayName || '');
  const [isSaving, setIsSaving] = useState(false);
  const [successMessage, setSuccessMessage] = useState('');

  const handleSaveProfile = async (e: React.FormEvent) => {
    e.preventDefault();
    setIsSaving(true);
    setSuccessMessage('');

    // Simulate API call
    await new Promise((resolve) => setTimeout(resolve, 500));

    setSuccessMessage('Profile updated successfully');
    setIsSaving(false);

    // Clear message after 3 seconds
    setTimeout(() => setSuccessMessage(''), 3000);
  };

  return (
    <motion.div
      variants={staggerContainer}
      initial="initial"
      animate="animate"
      className="max-w-4xl space-y-6"
    >
      {/* Header */}
      <motion.div variants={fadeInUp}>
        <h1 className="text-2xl font-bold text-white">Settings</h1>
        <p className="text-[hsl(228,10%,50%)] mt-1">
          Manage your account and preferences
        </p>
      </motion.div>

      {/* Profile Section */}
      <motion.div variants={fadeInUp}>
        <GlassCard>
          <div className="flex items-center gap-3 mb-6">
            <div
              className={cn(
                'h-10 w-10 rounded-xl flex items-center justify-center',
                'bg-[hsl(187,100%,50%,0.1)]'
              )}
            >
              <User className="h-5 w-5 text-[hsl(187,100%,50%)]" />
            </div>
            <div>
              <h2 className="text-lg font-semibold text-white">Profile</h2>
              <p className="text-sm text-[hsl(228,10%,50%)]">
                Your personal information
              </p>
            </div>
          </div>

          <form onSubmit={handleSaveProfile} className="space-y-4">
            {successMessage && (
              <motion.div
                initial={{ opacity: 0, y: -10 }}
                animate={{ opacity: 1, y: 0 }}
                className={cn(
                  'flex items-center gap-2 rounded-xl px-4 py-3 text-sm',
                  'bg-[hsl(160,84%,39%,0.1)]',
                  'border border-[hsl(160,84%,39%,0.3)]',
                  'text-[hsl(160,84%,55%)]'
                )}
              >
                <CheckCircle className="h-4 w-4" />
                {successMessage}
              </motion.div>
            )}

            <div className="grid grid-cols-1 md:grid-cols-2 gap-4">
              <Input
                label="Display Name"
                value={displayName}
                onChange={(e) => setDisplayName(e.target.value)}
                placeholder="Your name"
              />
              <Input
                label="Username"
                value={user?.username || ''}
                disabled
                helperText="Username cannot be changed"
              />
            </div>

            <Button
              type="submit"
              isLoading={isSaving}
              loadingText="Saving..."
              leftIcon={<Save className="h-4 w-4" />}
            >
              Save Changes
            </Button>
          </form>
        </GlassCard>
      </motion.div>

      {/* Security Section */}
      <motion.div variants={fadeInUp}>
        <GlassCard>
          <div className="flex items-center gap-3 mb-6">
            <div
              className={cn(
                'h-10 w-10 rounded-xl flex items-center justify-center',
                'bg-[hsl(160,84%,39%,0.1)]'
              )}
            >
              <Key className="h-5 w-5 text-[hsl(160,84%,45%)]" />
            </div>
            <div>
              <h2 className="text-lg font-semibold text-white">Security</h2>
              <p className="text-sm text-[hsl(228,10%,50%)]">
                Password and authentication settings
              </p>
            </div>
          </div>

          <div className="space-y-4">
            <div className="flex items-center justify-between py-3 border-b border-white/[0.06]">
              <div>
                <p className="font-medium text-white">Password</p>
                <p className="text-sm text-[hsl(228,10%,50%)]">
                  Last changed 30 days ago
                </p>
              </div>
              <Button variant="secondary" size="sm">
                Change Password
              </Button>
            </div>

            <div className="flex items-center justify-between py-3">
              <div>
                <p className="font-medium text-white">
                  Two-Factor Authentication
                </p>
                <p className="text-sm text-[hsl(228,10%,50%)]">
                  Add an extra layer of security
                </p>
              </div>
              <Badge variant="outline">Coming Soon</Badge>
            </div>
          </div>
        </GlassCard>
      </motion.div>

      {/* Notifications Section */}
      <motion.div variants={fadeInUp}>
        <GlassCard>
          <div className="flex items-center gap-3 mb-6">
            <div
              className={cn(
                'h-10 w-10 rounded-xl flex items-center justify-center',
                'bg-[hsl(38,92%,50%,0.1)]'
              )}
            >
              <Bell className="h-5 w-5 text-[hsl(38,92%,50%)]" />
            </div>
            <div>
              <h2 className="text-lg font-semibold text-white">Notifications</h2>
              <p className="text-sm text-[hsl(228,10%,50%)]">
                Configure how you receive alerts
              </p>
            </div>
          </div>

          <div className="space-y-4">
            <div className="flex items-center justify-between py-3 border-b border-white/[0.06]">
              <div>
                <p className="font-medium text-white">Device Offline Alerts</p>
                <p className="text-sm text-[hsl(228,10%,50%)]">
                  Get notified when a device goes offline
                </p>
              </div>
              <Toggle disabled />
            </div>

            <div className="flex items-center justify-between py-3 border-b border-white/[0.06]">
              <div>
                <p className="font-medium text-white">High CO₂ Warnings</p>
                <p className="text-sm text-[hsl(228,10%,50%)]">
                  Alert when CO₂ levels exceed 1000 ppm
                </p>
              </div>
              <Toggle disabled />
            </div>

            <div className="flex items-center justify-between py-3">
              <div>
                <p className="font-medium text-white">Daily Summary</p>
                <p className="text-sm text-[hsl(228,10%,50%)]">
                  Receive a daily digest of your home status
                </p>
              </div>
              <Toggle disabled />
            </div>
          </div>
        </GlassCard>
      </motion.div>

      {/* Danger Zone */}
      <motion.div variants={fadeInUp}>
        <GlassCard className="border-[hsl(0,72%,51%,0.3)]">
          <div className="flex items-center gap-3 mb-6">
            <div
              className={cn(
                'h-10 w-10 rounded-xl flex items-center justify-center',
                'bg-[hsl(0,72%,51%,0.1)]'
              )}
            >
              <Shield className="h-5 w-5 text-[hsl(0,72%,51%)]" />
            </div>
            <div>
              <h2 className="text-lg font-semibold text-[hsl(0,72%,60%)]">
                Danger Zone
              </h2>
              <p className="text-sm text-[hsl(228,10%,50%)]">
                Irreversible account actions
              </p>
            </div>
          </div>

          <div className="flex items-center justify-between">
            <div>
              <p className="font-medium text-white">Delete Account</p>
              <p className="text-sm text-[hsl(228,10%,50%)]">
                Permanently delete your account and all data
              </p>
            </div>
            <Button variant="destructive" size="sm">
              Delete Account
            </Button>
          </div>
        </GlassCard>
      </motion.div>
    </motion.div>
  );
}
