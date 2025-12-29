/**
 * API Client utilities for token management
 */

const TOKEN_KEY = 'accessToken';
const REFRESH_TOKEN_KEY = 'refreshToken';
const TOKEN_EXPIRY_KEY = 'tokenExpiry';

export function getAccessToken(): string | null {
  return localStorage.getItem(TOKEN_KEY);
}

export function getRefreshToken(): string | null {
  return localStorage.getItem(REFRESH_TOKEN_KEY);
}

export function setTokens(
  accessToken: string,
  refreshToken: string,
  expiresIn: number = 900 // 15 minutes default
): void {
  localStorage.setItem(TOKEN_KEY, accessToken);
  localStorage.setItem(REFRESH_TOKEN_KEY, refreshToken);
  localStorage.setItem(
    TOKEN_EXPIRY_KEY,
    String(Date.now() + expiresIn * 1000)
  );
}

export function clearTokens(): void {
  localStorage.removeItem(TOKEN_KEY);
  localStorage.removeItem(REFRESH_TOKEN_KEY);
  localStorage.removeItem(TOKEN_EXPIRY_KEY);
}

export function isTokenExpired(): boolean {
  const expiry = localStorage.getItem(TOKEN_EXPIRY_KEY);
  if (!expiry) return true;

  // Consider token expired 60 seconds before actual expiry
  return Date.now() > parseInt(expiry, 10) - 60000;
}
