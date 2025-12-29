import { useState } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { useAuth } from '@/context/auth-context';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import { Home, Eye, EyeOff } from 'lucide-react';

export function RegisterPage() {
  const { register } = useAuth();
  const navigate = useNavigate();
  const [username, setUsername] = useState('');
  const [displayName, setDisplayName] = useState('');
  const [password, setPassword] = useState('');
  const [confirmPassword, setConfirmPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [error, setError] = useState('');
  const [isLoading, setIsLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError('');

    if (password !== confirmPassword) {
      setError('Passwords do not match');
      return;
    }

    if (password.length < 8) {
      setError('Password must be at least 8 characters');
      return;
    }

    setIsLoading(true);

    try {
      await register(username, password, displayName);
      navigate('/dashboard');
    } catch (err) {
      setError(err instanceof Error ? err.message : 'Registration failed');
    } finally {
      setIsLoading(false);
    }
  };

  return (
    <div className="min-h-screen flex items-center justify-center bg-[hsl(40,20%,96%)] px-4 py-12">
      {/* Decorative background elements */}
      <div className="fixed inset-0 overflow-hidden pointer-events-none">
        <div className="absolute -top-1/4 -left-1/4 w-[800px] h-[800px] rounded-full bg-[hsl(140,25%,92%)] opacity-40 blur-3xl" />
        <div className="absolute -bottom-1/4 -right-1/4 w-[600px] h-[600px] rounded-full bg-[hsl(25,40%,92%)] opacity-30 blur-3xl" />
      </div>

      <div className="w-full max-w-md relative">
        {/* Logo */}
        <div className="flex flex-col items-center mb-8">
          <div className="flex h-14 w-14 items-center justify-center rounded-2xl bg-[hsl(25,60%,45%)] shadow-lg mb-4">
            <Home className="h-7 w-7 text-white" />
          </div>
          <h1 className="text-2xl font-semibold text-[hsl(30,10%,15%)] tracking-tight">
            PaperHome
          </h1>
          <p className="text-sm text-[hsl(30,10%,50%)] mt-1">
            Create your account
          </p>
        </div>

        <Card className="shadow-lg border-[hsl(30,15%,88%)]">
          <CardHeader className="text-center pb-2">
            <CardTitle className="text-xl">Get started</CardTitle>
            <CardDescription>
              Set up your smart home dashboard
            </CardDescription>
          </CardHeader>

          <CardContent>
            <form onSubmit={handleSubmit} className="space-y-4">
              {error && (
                <div className="rounded-lg bg-[hsl(0,40%,95%)] border border-[hsl(0,40%,85%)] px-4 py-3 text-sm text-[hsl(0,50%,40%)]">
                  {error}
                </div>
              )}

              <Input
                label="Display Name"
                type="text"
                value={displayName}
                onChange={(e) => setDisplayName(e.target.value)}
                placeholder="How should we call you?"
                required
                autoFocus
              />

              <Input
                label="Username"
                type="text"
                value={username}
                onChange={(e) => setUsername(e.target.value.toLowerCase().replace(/[^a-z0-9_]/g, ''))}
                placeholder="Choose a username"
                required
                autoComplete="username"
              />

              <div className="relative">
                <Input
                  label="Password"
                  type={showPassword ? 'text' : 'password'}
                  value={password}
                  onChange={(e) => setPassword(e.target.value)}
                  placeholder="Minimum 8 characters"
                  required
                  autoComplete="new-password"
                />
                <button
                  type="button"
                  onClick={() => setShowPassword(!showPassword)}
                  className="absolute right-3 top-[34px] text-[hsl(30,10%,50%)] hover:text-[hsl(30,10%,30%)] transition-colors"
                >
                  {showPassword ? (
                    <EyeOff className="h-4 w-4" />
                  ) : (
                    <Eye className="h-4 w-4" />
                  )}
                </button>
              </div>

              <Input
                label="Confirm Password"
                type={showPassword ? 'text' : 'password'}
                value={confirmPassword}
                onChange={(e) => setConfirmPassword(e.target.value)}
                placeholder="Re-enter your password"
                required
                autoComplete="new-password"
              />

              <Button
                type="submit"
                className="w-full"
                disabled={isLoading}
              >
                {isLoading ? 'Creating account...' : 'Create account'}
              </Button>
            </form>

            <div className="mt-6 text-center text-sm text-[hsl(30,10%,45%)]">
              Already have an account?{' '}
              <Link
                to="/login"
                className="font-medium text-[hsl(25,60%,45%)] hover:text-[hsl(25,60%,35%)] transition-colors"
              >
                Sign in
              </Link>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
