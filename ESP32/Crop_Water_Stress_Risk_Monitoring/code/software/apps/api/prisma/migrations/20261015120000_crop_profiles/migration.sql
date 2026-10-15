CREATE TABLE "CropProfile" (
  "id" TEXT NOT NULL,
  "organizationId" TEXT NOT NULL,
  "name" TEXT NOT NULL,
  "cultivar" TEXT,
  "description" TEXT,
  "thresholds" JSONB NOT NULL,
  "active" BOOLEAN NOT NULL DEFAULT true,
  "createdAt" TIMESTAMP(3) NOT NULL DEFAULT CURRENT_TIMESTAMP,
  "updatedAt" TIMESTAMP(3) NOT NULL,
  CONSTRAINT "CropProfile_pkey" PRIMARY KEY ("id")
);

ALTER TABLE "Plot" ADD COLUMN "cropProfileId" TEXT;
CREATE UNIQUE INDEX "CropProfile_organizationId_name_key" ON "CropProfile"("organizationId", "name");
ALTER TABLE "CropProfile" ADD CONSTRAINT "CropProfile_organizationId_fkey" FOREIGN KEY ("organizationId") REFERENCES "Organization"("id") ON DELETE CASCADE ON UPDATE CASCADE;
ALTER TABLE "Plot" ADD CONSTRAINT "Plot_cropProfileId_fkey" FOREIGN KEY ("cropProfileId") REFERENCES "CropProfile"("id") ON DELETE SET NULL ON UPDATE CASCADE;
