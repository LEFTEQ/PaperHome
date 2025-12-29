import { useState } from 'react';
import { useAuth } from '@/context/auth-context';
import { Card, CardContent, CardHeader, CardTitle, CardDescription } from '@/components/ui/card';
import { Button } from '@/components/ui/button';
import { Input } from '@/components/ui/input';
import { Badge } from '@/components/ui/badge';
import { User, Key, Shield, Bell, Save } from 'lucide-react';

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
  };

  return (
    <div className="p-8 max-w-4xl">
      {/* Header */}
      <div className="mb-8">
        <h1 className="text-2xl font-semibold text-[hsl(30,10%,15%)]">
          Settings
        </h1>
        <p className="text-[hsl(30,10%,45%)] mt-1">
          Manage your account and preferences
        </p>
      </div>

      <div className="space-y-6">
        {/* Profile Section */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-3">
              <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-[hsl(25,50%,92%)]">
                <User className="h-5 w-5 text-[hsl(25,60%,45%)]" />
              </div>
              <div>
                <CardTitle>Profile</CardTitle>
                <CardDescription>
                  Your personal information
                </CardDescription>
              </div>
            </div>
          </CardHeader>
          <CardContent>
            <form onSubmit={handleSaveProfile} className="space-y-4">
              {successMessage && (
                <div className="rounded-lg bg-[hsl(140,30%,94%)] border border-[hsl(140,30%,85%)] px-4 py-3 text-sm text-[hsl(140,30%,30%)]">
                  {successMessage}
                </div>
              )}

              <div className="grid grid-cols-2 gap-4">
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
                  className="opacity-60"
                />
              </div>

              <Button type="submit" disabled={isSaving}>
                <Save className="h-4 w-4 mr-2" />
                {isSaving ? 'Saving...' : 'Save Changes'}
              </Button>
            </form>
          </CardContent>
        </Card>

        {/* Security Section */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-3">
              <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-[hsl(140,30%,92%)]">
                <Key className="h-5 w-5 text-[hsl(140,30%,40%)]" />
              </div>
              <div>
                <CardTitle>Security</CardTitle>
                <CardDescription>
                  Password and authentication settings
                </CardDescription>
              </div>
            </div>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              <div className="flex items-center justify-between py-3 border-b border-[hsl(30,15%,90%)]">
                <div>
                  <p className="font-medium text-[hsl(30,10%,20%)]">Password</p>
                  <p className="text-sm text-[hsl(30,10%,50%)]">
                    Last changed 30 days ago
                  </p>
                </div>
                <Button variant="outline" size="sm">
                  Change Password
                </Button>
              </div>

              <div className="flex items-center justify-between py-3">
                <div>
                  <p className="font-medium text-[hsl(30,10%,20%)]">
                    Two-Factor Authentication
                  </p>
                  <p className="text-sm text-[hsl(30,10%,50%)]">
                    Add an extra layer of security
                  </p>
                </div>
                <Badge variant="outline">Coming Soon</Badge>
              </div>
            </div>
          </CardContent>
        </Card>

        {/* Notifications Section */}
        <Card>
          <CardHeader>
            <div className="flex items-center gap-3">
              <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-[hsl(200,40%,92%)]">
                <Bell className="h-5 w-5 text-[hsl(200,40%,45%)]" />
              </div>
              <div>
                <CardTitle>Notifications</CardTitle>
                <CardDescription>
                  Configure how you receive alerts
                </CardDescription>
              </div>
            </div>
          </CardHeader>
          <CardContent>
            <div className="space-y-4">
              <div className="flex items-center justify-between py-3 border-b border-[hsl(30,15%,90%)]">
                <div>
                  <p className="font-medium text-[hsl(30,10%,20%)]">
                    Device Offline Alerts
                  </p>
                  <p className="text-sm text-[hsl(30,10%,50%)]">
                    Get notified when a device goes offline
                  </p>
                </div>
                <Badge variant="outline">Coming Soon</Badge>
              </div>

              <div className="flex items-center justify-between py-3 border-b border-[hsl(30,15%,90%)]">
                <div>
                  <p className="font-medium text-[hsl(30,10%,20%)]">
                    High CO2 Warnings
                  </p>
                  <p className="text-sm text-[hsl(30,10%,50%)]">
                    Alert when CO2 levels exceed 1000 ppm
                  </p>
                </div>
                <Badge variant="outline">Coming Soon</Badge>
              </div>

              <div className="flex items-center justify-between py-3">
                <div>
                  <p className="font-medium text-[hsl(30,10%,20%)]">
                    Daily Summary
                  </p>
                  <p className="text-sm text-[hsl(30,10%,50%)]">
                    Receive a daily digest of your home status
                  </p>
                </div>
                <Badge variant="outline">Coming Soon</Badge>
              </div>
            </div>
          </CardContent>
        </Card>

        {/* Danger Zone */}
        <Card className="border-[hsl(0,40%,85%)]">
          <CardHeader>
            <div className="flex items-center gap-3">
              <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-[hsl(0,40%,94%)]">
                <Shield className="h-5 w-5 text-[hsl(0,50%,50%)]" />
              </div>
              <div>
                <CardTitle className="text-[hsl(0,50%,45%)]">Danger Zone</CardTitle>
                <CardDescription>
                  Irreversible account actions
                </CardDescription>
              </div>
            </div>
          </CardHeader>
          <CardContent>
            <div className="flex items-center justify-between">
              <div>
                <p className="font-medium text-[hsl(30,10%,20%)]">
                  Delete Account
                </p>
                <p className="text-sm text-[hsl(30,10%,50%)]">
                  Permanently delete your account and all data
                </p>
              </div>
              <Button variant="destructive" size="sm">
                Delete Account
              </Button>
            </div>
          </CardContent>
        </Card>
      </div>
    </div>
  );
}
