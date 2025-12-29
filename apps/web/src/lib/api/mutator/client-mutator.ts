/**
 * Custom Fetch Mutator for Orval
 * Handles authentication and token refresh
 */

import {
  getAccessToken,
  getRefreshToken,
  setTokens,
  clearTokens,
  isTokenExpired,
} from '../client';

const API_BASE_URL = import.meta.env.VITE_API_URL || '/api/v1';

export type ErrorType<Error> = Error & { status: number };

// Prevent concurrent refresh calls
let refreshPromise: Promise<boolean> | null = null;

async function refreshAccessToken(): Promise<boolean> {
  if (refreshPromise) {
    return refreshPromise;
  }

  const refreshToken = getRefreshToken();
  if (!refreshToken) {
    return false;
  }

  refreshPromise = (async () => {
    try {
      const response = await fetch(`${API_BASE_URL}/auth/refresh`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json',
        },
        body: JSON.stringify({ refreshToken }),
      });

      if (!response.ok) {
        clearTokens();
        window.location.href = '/login';
        return false;
      }

      const data = await response.json();
      setTokens(data.accessToken, data.refreshToken);
      return true;
    } catch (error) {
      console.error('Token refresh failed:', error);
      clearTokens();
      window.location.href = '/login';
      return false;
    } finally {
      refreshPromise = null;
    }
  })();

  return refreshPromise;
}

/**
 * Custom fetch mutator for Orval-generated hooks
 */
export const customFetch = async <T>(
  url: string,
  options?: RequestInit
): Promise<T> => {
  const fullUrl = `${API_BASE_URL}${url}`;

  const headers: Record<string, string> = {
    'Content-Type': 'application/json',
    ...(options?.headers as Record<string, string>),
  };

  // Check if token needs refresh
  if (typeof window !== 'undefined') {
    if (isTokenExpired()) {
      await refreshAccessToken();
    }

    const token = getAccessToken();
    if (token) {
      headers['Authorization'] = `Bearer ${token}`;
    }
  }

  const response = await fetch(fullUrl, {
    ...options,
    headers,
  });

  if (!response.ok) {
    const errorData = await response.json().catch(() => ({
      message: response.statusText,
    }));

    const error = new Error(
      Array.isArray(errorData.message)
        ? errorData.message.join(', ')
        : errorData.message || 'Request failed'
    ) as ErrorType<Error>;
    error.status = response.status;
    throw error;
  }

  const text = await response.text();
  const data = text ? JSON.parse(text) : {};

  return {
    data,
    status: response.status,
    headers: response.headers,
  } as T;
};

export default customFetch;
