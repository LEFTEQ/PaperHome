import {
  Injectable,
  UnauthorizedException,
  ConflictException,
} from '@nestjs/common';
import { ConfigService } from '@nestjs/config';
import { JwtService } from '@nestjs/jwt';
import { InjectRepository } from '@nestjs/typeorm';
import { Repository } from 'typeorm';
import * as bcrypt from 'bcrypt';
import { User } from '../../entities/user.entity';
import { RefreshToken } from '../../entities/refresh-token.entity';
import { RegisterDto } from './dto';
import { JwtPayload } from './strategies/jwt.strategy';

@Injectable()
export class AuthService {
  constructor(
    @InjectRepository(User)
    private userRepository: Repository<User>,
    @InjectRepository(RefreshToken)
    private refreshTokenRepository: Repository<RefreshToken>,
    private jwtService: JwtService,
    private configService: ConfigService
  ) {}

  async register(registerDto: RegisterDto): Promise<Omit<User, 'passwordHash'>> {
    const { username, password, displayName } = registerDto;

    const existingUser = await this.userRepository.findOne({
      where: { username },
    });

    if (existingUser) {
      throw new ConflictException('Username already exists');
    }

    const passwordHash = await bcrypt.hash(password, 10);

    const user = this.userRepository.create({
      username,
      passwordHash,
      displayName,
    });

    const savedUser = await this.userRepository.save(user);

    const { passwordHash: _, ...userWithoutPassword } = savedUser;
    return userWithoutPassword as Omit<User, 'passwordHash'>;
  }

  async validateUser(username: string, password: string): Promise<User | null> {
    const user = await this.userRepository.findOne({ where: { username } });

    if (!user || !user.isActive) {
      return null;
    }

    const isPasswordValid = await bcrypt.compare(password, user.passwordHash);

    if (!isPasswordValid) {
      return null;
    }

    return user;
  }

  async login(user: User): Promise<{
    accessToken: string;
    refreshToken: string;
    user: Omit<User, 'passwordHash'>;
  }> {
    const payload: JwtPayload = {
      sub: user.id,
      username: user.username,
    };

    const accessToken = this.jwtService.sign(payload, {
      secret: this.configService.get<string>('JWT_SECRET') || 'dev-secret-key',
      expiresIn: this.configService.get('JWT_EXPIRES_IN') || '15m',
    });

    const refreshToken = this.jwtService.sign(payload, {
      secret:
        this.configService.get<string>('JWT_REFRESH_SECRET') ||
        'dev-refresh-secret-key',
      expiresIn: this.configService.get('JWT_REFRESH_EXPIRES_IN') || '7d',
    });

    const tokenHash = await bcrypt.hash(refreshToken, 10);

    const expiresAt = new Date();
    expiresAt.setDate(expiresAt.getDate() + 7);

    await this.refreshTokenRepository.save({
      userId: user.id,
      tokenHash,
      expiresAt,
    });

    const { passwordHash: _, ...userWithoutPassword } = user;

    return {
      accessToken,
      refreshToken,
      user: userWithoutPassword as Omit<User, 'passwordHash'>,
    };
  }

  async refreshAccessToken(refreshToken: string): Promise<{
    accessToken: string;
    refreshToken: string;
  }> {
    try {
      const payload = this.jwtService.verify<JwtPayload>(refreshToken, {
        secret:
          this.configService.get<string>('JWT_REFRESH_SECRET') ||
          'dev-refresh-secret-key',
      });

      const user = await this.userRepository.findOne({
        where: { id: payload.sub },
      });

      if (!user || !user.isActive) {
        throw new UnauthorizedException('User not found or inactive');
      }

      const storedTokens = await this.refreshTokenRepository.find({
        where: { userId: user.id },
      });

      let isValidToken = false;
      for (const storedToken of storedTokens) {
        const isMatch = await bcrypt.compare(refreshToken, storedToken.tokenHash);
        if (isMatch && storedToken.expiresAt > new Date()) {
          isValidToken = true;
          await this.refreshTokenRepository.delete(storedToken.id);
          break;
        }
      }

      if (!isValidToken) {
        throw new UnauthorizedException('Invalid refresh token');
      }

      const newPayload: JwtPayload = {
        sub: user.id,
        username: user.username,
      };

      const accessToken = this.jwtService.sign(newPayload, {
        secret:
          this.configService.get<string>('JWT_SECRET') || 'dev-secret-key',
        expiresIn: this.configService.get('JWT_EXPIRES_IN') || '15m',
      });

      const newRefreshToken = this.jwtService.sign(newPayload, {
        secret:
          this.configService.get<string>('JWT_REFRESH_SECRET') ||
          'dev-refresh-secret-key',
        expiresIn: this.configService.get('JWT_REFRESH_EXPIRES_IN') || '7d',
      });

      const tokenHash = await bcrypt.hash(newRefreshToken, 10);

      const expiresAt = new Date();
      expiresAt.setDate(expiresAt.getDate() + 7);

      await this.refreshTokenRepository.save({
        userId: user.id,
        tokenHash,
        expiresAt,
      });

      return {
        accessToken,
        refreshToken: newRefreshToken,
      };
    } catch {
      throw new UnauthorizedException('Invalid or expired refresh token');
    }
  }

  async getProfile(userId: string): Promise<Omit<User, 'passwordHash'>> {
    const user = await this.userRepository.findOne({ where: { id: userId } });
    if (!user) {
      throw new UnauthorizedException('User not found');
    }
    const { passwordHash: _, ...userWithoutPassword } = user;
    return userWithoutPassword as Omit<User, 'passwordHash'>;
  }

  async logout(userId: string, refreshToken?: string): Promise<void> {
    if (refreshToken) {
      const storedTokens = await this.refreshTokenRepository.find({
        where: { userId },
      });

      for (const storedToken of storedTokens) {
        const isMatch = await bcrypt.compare(refreshToken, storedToken.tokenHash);
        if (isMatch) {
          await this.refreshTokenRepository.delete(storedToken.id);
          break;
        }
      }
    } else {
      await this.refreshTokenRepository.delete({ userId });
    }
  }
}
