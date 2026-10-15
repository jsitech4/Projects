CREATE TYPE "AlertStatus" AS ENUM ('OPEN', 'ACKNOWLEDGED', 'RESOLVED');

CREATE TABLE "Farm" (
  "id" TEXT NOT NULL,
  "organizationId" TEXT NOT NULL,
  "name" TEXT NOT NULL,
  "location" TEXT,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  CONSTRAINT "Farm_pkey" PRIMARY KEY ("id")
);
CREATE UNIQUE INDEX "Farm_organizationId_name_key" ON "Farm"("organizationId", "name");
ALTER TABLE "Farm" ADD CONSTRAINT "Farm_organizationId_fkey" FOREIGN KEY ("organizationId") REFERENCES "Organization"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Site" ADD COLUMN "farmId" TEXT;
ALTER TABLE "Site" ADD CONSTRAINT "Site_farmId_fkey" FOREIGN KEY ("farmId") REFERENCES "Farm"("id") ON DELETE SET NULL ON UPDATE CASCADE;

ALTER TABLE "AlertRule" ADD COLUMN "escalationMinutes" INTEGER NOT NULL DEFAULT 60;
CREATE TABLE "Alert" (
  "id" TEXT NOT NULL,
  "organizationId" TEXT NOT NULL,
  "deviceId" TEXT NOT NULL,
  "riskAssessmentId" TEXT,
  "title" TEXT NOT NULL,
  "severity" "RiskLevel" NOT NULL,
  "detail" TEXT NOT NULL,
  "status" "AlertStatus" NOT NULL DEFAULT 'OPEN',
  "acknowledgedAt" TIMESTAMP(3),
  "acknowledgedById" TEXT,
  "resolvedAt" TIMESTAMP(3),
  "escalatedAt" TIMESTAMP(3),
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "Alert_pkey" PRIMARY KEY ("id")
);
CREATE INDEX "Alert_organizationId_status_createdAt_idx" ON "Alert"("organizationId", "status", "createdAt");
CREATE INDEX "Alert_deviceId_status_idx" ON "Alert"("deviceId", "status");
ALTER TABLE "Alert" ADD CONSTRAINT "Alert_organizationId_fkey" FOREIGN KEY ("organizationId") REFERENCES "Organization"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Alert" ADD CONSTRAINT "Alert_deviceId_fkey" FOREIGN KEY ("deviceId") REFERENCES "Device"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "NotificationPreference" ADD CONSTRAINT "NotificationPreference_userId_fkey" FOREIGN KEY ("userId") REFERENCES "User"("id") ON DELETE CASCADE ON UPDATE CASCADE;
