import { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { motion } from 'framer-motion';
import { Activity } from 'lucide-react';
import { useAuth } from '@/context/auth-context';
import { Button } from '@/components/ui/button';
import { Input, PasswordInput } from '@/components/ui/input';
import { GlassCard } from '@/components/ui/glass-card';
import { AnimatedBackground } from '@/components/layout/AnimatedBackground';
import { pageTransition, staggerContainer, fadeInUp } from '@/lib/animations';
import { cn } from '@/lib/utils';

export function LoginPage() {
  const { login } = useAuth();
  const navigate = useNavigate();
  const [username, setUsername] = useState('');
  const [password, setPassword] = useState('');
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');
    setIsLoading(true);

    try {
      await login(username, password);
      navigate('/dashboard');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Login failed');
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen flex items-center justify-center px-4">
      {/* Animated gradient background */}
      <AnimatedBackground />

      <motion.div
        className="w-full max-w-md relative z-10"
        variants={staggerContainer}
        initial="initial"
        animate="animate"
      >
        {/* Logo */}
        <motion.div
          className="flex flex-col items-center mb-8"
          variants={fadeInUp}
        >
          <div
            className={cn(
              'h-16 w-16 rounded-2xl mb-4',
              'bg-gradient-to-br from-accent to-success',
              'flex items-center justify-center',
              'shadow-[0_0_40px_rgb(0_229_255/0.4)]'
            )}
          >
            <Activity className="h-8 w-8 text-bg-base" />
          </div>
          <h1 className="text-2xl font-bold text-white tracking-tight">
            PaperHome
          </h1>
          <p className="text-sm text-text-muted mt-1">
            Smart Home Dashboard
          </p>
        </motion.div>

        {/* Login Card */}
        <motion.div variants={fadeInUp}>
          <GlassCard variant="elevated" size="lg" className="backdrop-blur-2xl">
            <div className="text-center mb-6">
              <h2 className="text-xl font-semibold text-white">Welcome back</h2>
              <p className="text-sm text-text-muted mt-1">
                Sign in to your account to continue
              </p>
            </div>

            <form onSubmit={handleSubmit} className="space-y-5">
              {error && (
                <motion.div
                  initial={{ opacity: 0, y: -10 }}
                  animate={{ opacity: 1, y: 0 }}
                  className={cn(
                    'rounded-xl px-4 py-3 text-sm',
                    'bg-error-bg',
                    'border border-error/30',
                    'text-error'
                  )}
                >
                  {error}
                </motion.div>
              )}

              <Input
                label="Username"
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value)}
                placeholder="Enter your username"
                required
                autoComplete="username"
                autoFocus
              />

              <PasswordInput
                label="Password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                placeholder="Enter your password"
                required
                autoComplete="current-password"
              />

              <Button
                type="submit"
                fullWidth
                size="lg"
                isLoading={isLoading}
                loadingText="Signing in..."
              >
                Sign in
              </Button>
            </form>

            <div className="mt-6 text-center text-sm text-text-muted">
              Don't have an account?{' '}
              <Link
                to="/register"
                className="font-medium text-accent hover:text-accent-hover transition-colors"
              >
                Create one
              </Link>
            </div>
          </GlassCard>
        </motion.div>

        {/* Footer */}
        <motion.p
          className="mt-8 text-center text-xs text-text-subtle"
          variants={fadeInUp}
        >
          Monitor your home environment with ease
        </motion.p>
      </motion.div>
    </div>
  );
}
