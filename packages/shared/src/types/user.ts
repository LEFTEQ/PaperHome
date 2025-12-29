export interface User {
  id: string;
  username: string;
  displayName: string;
  isActive: boolean;
  createdAt: Date;
  updatedAt: Date;
}

export interface UserWithPassword extends User {
  passwordHash: string;
}

export interface CreateUserDto {
  username: string;
  password: string;
  displayName: string;
}

export interface UpdateUserDto {
  displayName?: string;
  password?: string;
}

export interface LoginDto {
  username: string;
  password: string;
}

export interface AuthResponse {
  accessToken: string;
  refreshToken: string;
  user: User;
}

export interface RefreshTokenDto {
  refreshToken: string;
}
