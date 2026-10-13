import argon2 from 'argon2';
import dotenv from 'dotenv';
import { resolve } from 'node:path';
import { PrismaClient, Role } from '@prisma/client';

dotenv.config({ path: process.env.DOTENV_CONFIG_PATH ?? resolve(process.cwd(), '../../.env') });

const db = new PrismaClient();
const email = process.env.INITIAL_OWNER_EMAIL;
const password = process.env.INITIAL_OWNER_PASSWORD;
if (!email || !password) throw new Error('Set INITIAL_OWNER_EMAIL and INITIAL_OWNER_PASSWORD only for this one-time seed operation.');
if (password.length < 14) throw new Error('INITIAL_OWNER_PASSWORD must be at least 14 characters.');
await db.user.upsert({ where: { email }, update: {}, create: { email, passwordHash: await argon2.hash(password), role: Role.OWNER } });
await db.$disconnect();
