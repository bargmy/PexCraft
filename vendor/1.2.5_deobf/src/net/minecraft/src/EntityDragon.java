package net.minecraft.src;

import java.util.Iterator;
import java.util.List;

public class EntityDragon extends EntityDragonBase {
	public double targetX;
	public double targetY;
	public double targetZ;
	public double[][] field_40162_d = new double[64][3];
	public int field_40164_e = -1;
	public EntityDragonPart[] dragonPartArray = new EntityDragonPart[]{this.dragonPartHead = new EntityDragonPart(this, "head", 6.0F, 6.0F), this.dragonPartBody = new EntityDragonPart(this, "body", 8.0F, 8.0F), this.dragonPartTail1 = new EntityDragonPart(this, "tail", 4.0F, 4.0F), this.dragonPartTail2 = new EntityDragonPart(this, "tail", 4.0F, 4.0F), this.dragonPartTail3 = new EntityDragonPart(this, "tail", 4.0F, 4.0F), this.dragonPartWing1 = new EntityDragonPart(this, "wing", 4.0F, 4.0F), this.dragonPartWing2 = new EntityDragonPart(this, "wing", 4.0F, 4.0F)};
	public EntityDragonPart dragonPartHead;
	public EntityDragonPart dragonPartBody;
	public EntityDragonPart dragonPartTail1;
	public EntityDragonPart dragonPartTail2;
	public EntityDragonPart dragonPartTail3;
	public EntityDragonPart dragonPartWing1;
	public EntityDragonPart dragonPartWing2;
	public float field_40173_aw = 0.0F;
	public float field_40172_ax = 0.0F;
	public boolean field_40163_ay = false;
	public boolean field_40161_az = false;
	private Entity target;
	public int field_40178_aA = 0;
	public EntityEnderCrystal healingEnderCrystal = null;

	public EntityDragon(World var1) {
		super(var1);
		this.maxHealth = 200;
		this.setEntityHealth(this.maxHealth);
		this.texture = "/mob/enderdragon/ender.png";
		this.setSize(16.0F, 8.0F);
		this.noClip = true;
		this.isImmuneToFire = true;
		this.targetY = 100.0D;
		this.ignoreFrustumCheck = true;
	}

	protected void entityInit() {
		super.entityInit();
		this.dataWatcher.addObject(16, new Integer(this.maxHealth));
	}

	public double[] func_40160_a(int var1, float var2) {
		if(this.health <= 0) {
			var2 = 0.0F;
		}

		var2 = 1.0F - var2;
		int var3 = this.field_40164_e - var1 * 1 & 63;
		int var4 = this.field_40164_e - var1 * 1 - 1 & 63;
		double[] var5 = new double[3];
		double var6 = this.field_40162_d[var3][0];

		double var8;
		for(var8 = this.field_40162_d[var4][0] - var6; var8 < -180.0D; var8 += 360.0D) {
		}

		while(var8 >= 180.0D) {
			var8 -= 360.0D;
		}

		var5[0] = var6 + var8 * (double)var2;
		var6 = this.field_40162_d[var3][1];
		var8 = this.field_40162_d[var4][1] - var6;
		var5[1] = var6 + var8 * (double)var2;
		var5[2] = this.field_40162_d[var3][2] + (this.field_40162_d[var4][2] - this.field_40162_d[var3][2]) * (double)var2;
		return var5;
	}

	public void onLivingUpdate() {
		this.field_40173_aw = this.field_40172_ax;
		if(!this.worldObj.isRemote) {
			this.dataWatcher.updateObject(16, Integer.valueOf(this.health));
		}

		float var1;
		float var3;
		float var26;
		if(this.health <= 0) {
			var1 = (this.rand.nextFloat() - 0.5F) * 8.0F;
			var26 = (this.rand.nextFloat() - 0.5F) * 4.0F;
			var3 = (this.rand.nextFloat() - 0.5F) * 8.0F;
			this.worldObj.spawnParticle("largeexplode", this.posX + (double)var1, this.posY + 2.0D + (double)var26, this.posZ + (double)var3, 0.0D, 0.0D, 0.0D);
		} else {
			this.updateDragonEnderCrystal();
			var1 = 0.2F / (MathHelper.sqrt_double(this.motionX * this.motionX + this.motionZ * this.motionZ) * 10.0F + 1.0F);
			var1 *= (float)Math.pow(2.0D, this.motionY);
			if(this.field_40161_az) {
				this.field_40172_ax += var1 * 0.5F;
			} else {
				this.field_40172_ax += var1;
			}

			while(this.rotationYaw >= 180.0F) {
				this.rotationYaw -= 360.0F;
			}

			while(this.rotationYaw < -180.0F) {
				this.rotationYaw += 360.0F;
			}

			if(this.field_40164_e < 0) {
				for(int var2 = 0; var2 < this.field_40162_d.length; ++var2) {
					this.field_40162_d[var2][0] = (double)this.rotationYaw;
					this.field_40162_d[var2][1] = this.posY;
				}
			}

			if(++this.field_40164_e == this.field_40162_d.length) {
				this.field_40164_e = 0;
			}

			this.field_40162_d[this.field_40164_e][0] = (double)this.rotationYaw;
			this.field_40162_d[this.field_40164_e][1] = this.posY;
			double var4;
			double var6;
			double var8;
			double var25;
			float var31;
			if(this.worldObj.isRemote) {
				if(this.newPosRotationIncrements > 0) {
					var25 = this.posX + (this.newPosX - this.posX) / (double)this.newPosRotationIncrements;
					var4 = this.posY + (this.newPosY - this.posY) / (double)this.newPosRotationIncrements;
					var6 = this.posZ + (this.newPosZ - this.posZ) / (double)this.newPosRotationIncrements;

					for(var8 = this.newRotationYaw - (double)this.rotationYaw; var8 < -180.0D; var8 += 360.0D) {
					}

					while(var8 >= 180.0D) {
						var8 -= 360.0D;
					}

					this.rotationYaw = (float)((double)this.rotationYaw + var8 / (double)this.newPosRotationIncrements);
					this.rotationPitch = (float)((double)this.rotationPitch + (this.newRotationPitch - (double)this.rotationPitch) / (double)this.newPosRotationIncrements);
					--this.newPosRotationIncrements;
					this.setPosition(var25, var4, var6);
					this.setRotation(this.rotationYaw, this.rotationPitch);
				}
			} else {
				var25 = this.targetX - this.posX;
				var4 = this.targetY - this.posY;
				var6 = this.targetZ - this.posZ;
				var8 = var25 * var25 + var4 * var4 + var6 * var6;
				if(this.target != null) {
					this.targetX = this.target.posX;
					this.targetZ = this.target.posZ;
					double var10 = this.targetX - this.posX;
					double var12 = this.targetZ - this.posZ;
					double var14 = Math.sqrt(var10 * var10 + var12 * var12);
					double var16 = (double)0.4F + var14 / 80.0D - 1.0D;
					if(var16 > 10.0D) {
						var16 = 10.0D;
					}

					this.targetY = this.target.boundingBox.minY + var16;
				} else {
					this.targetX += this.rand.nextGaussian() * 2.0D;
					this.targetZ += this.rand.nextGaussian() * 2.0D;
				}

				if(this.field_40163_ay || var8 < 100.0D || var8 > 22500.0D || this.isCollidedHorizontally || this.isCollidedVertically) {
					this.func_41006_aA();
				}

				var4 /= (double)MathHelper.sqrt_double(var25 * var25 + var6 * var6);
				var31 = 0.6F;
				if(var4 < (double)(-var31)) {
					var4 = (double)(-var31);
				}

				if(var4 > (double)var31) {
					var4 = (double)var31;
				}

				for(this.motionY += var4 * (double)0.1F; this.rotationYaw < -180.0F; this.rotationYaw += 360.0F) {
				}

				while(this.rotationYaw >= 180.0F) {
					this.rotationYaw -= 360.0F;
				}

				double var11 = 180.0D - Math.atan2(var25, var6) * 180.0D / (double)((float)Math.PI);

				double var13;
				for(var13 = var11 - (double)this.rotationYaw; var13 < -180.0D; var13 += 360.0D) {
				}

				while(var13 >= 180.0D) {
					var13 -= 360.0D;
				}

				if(var13 > 50.0D) {
					var13 = 50.0D;
				}

				if(var13 < -50.0D) {
					var13 = -50.0D;
				}

				Vec3D var15 = Vec3D.createVector(this.targetX - this.posX, this.targetY - this.posY, this.targetZ - this.posZ).normalize();
				Vec3D var39 = Vec3D.createVector((double)MathHelper.sin(this.rotationYaw * (float)Math.PI / 180.0F), this.motionY, (double)(-MathHelper.cos(this.rotationYaw * (float)Math.PI / 180.0F))).normalize();
				float var17 = (float)(var39.dotProduct(var15) + 0.5D) / 1.5F;
				if(var17 < 0.0F) {
					var17 = 0.0F;
				}

				this.randomYawVelocity *= 0.8F;
				float var18 = MathHelper.sqrt_double(this.motionX * this.motionX + this.motionZ * this.motionZ) * 1.0F + 1.0F;
				double var19 = Math.sqrt(this.motionX * this.motionX + this.motionZ * this.motionZ) * 1.0D + 1.0D;
				if(var19 > 40.0D) {
					var19 = 40.0D;
				}

				this.randomYawVelocity = (float)((double)this.randomYawVelocity + var13 * ((double)0.7F / var19 / (double)var18));
				this.rotationYaw += this.randomYawVelocity * 0.1F;
				float var21 = (float)(2.0D / (var19 + 1.0D));
				float var22 = 0.06F;
				this.moveFlying(0.0F, -1.0F, var22 * (var17 * var21 + (1.0F - var21)));
				if(this.field_40161_az) {
					this.moveEntity(this.motionX * (double)0.8F, this.motionY * (double)0.8F, this.motionZ * (double)0.8F);
				} else {
					this.moveEntity(this.motionX, this.motionY, this.motionZ);
				}

				Vec3D var23 = Vec3D.createVector(this.motionX, this.motionY, this.motionZ).normalize();
				float var24 = (float)(var23.dotProduct(var39) + 1.0D) / 2.0F;
				var24 = 0.8F + 0.15F * var24;
				this.motionX *= (double)var24;
				this.motionZ *= (double)var24;
				this.motionY *= (double)0.91F;
			}

			this.renderYawOffset = this.rotationYaw;
			this.dragonPartHead.width = this.dragonPartHead.height = 3.0F;
			this.dragonPartTail1.width = this.dragonPartTail1.height = 2.0F;
			this.dragonPartTail2.width = this.dragonPartTail2.height = 2.0F;
			this.dragonPartTail3.width = this.dragonPartTail3.height = 2.0F;
			this.dragonPartBody.height = 3.0F;
			this.dragonPartBody.width = 5.0F;
			this.dragonPartWing1.height = 2.0F;
			this.dragonPartWing1.width = 4.0F;
			this.dragonPartWing2.height = 3.0F;
			this.dragonPartWing2.width = 4.0F;
			var26 = (float)(this.func_40160_a(5, 1.0F)[1] - this.func_40160_a(10, 1.0F)[1]) * 10.0F / 180.0F * (float)Math.PI;
			var3 = MathHelper.cos(var26);
			float var27 = -MathHelper.sin(var26);
			float var5 = this.rotationYaw * (float)Math.PI / 180.0F;
			float var28 = MathHelper.sin(var5);
			float var7 = MathHelper.cos(var5);
			this.dragonPartBody.onUpdate();
			this.dragonPartBody.setLocationAndAngles(this.posX + (double)(var28 * 0.5F), this.posY, this.posZ - (double)(var7 * 0.5F), 0.0F, 0.0F);
			this.dragonPartWing1.onUpdate();
			this.dragonPartWing1.setLocationAndAngles(this.posX + (double)(var7 * 4.5F), this.posY + 2.0D, this.posZ + (double)(var28 * 4.5F), 0.0F, 0.0F);
			this.dragonPartWing2.onUpdate();
			this.dragonPartWing2.setLocationAndAngles(this.posX - (double)(var7 * 4.5F), this.posY + 2.0D, this.posZ - (double)(var28 * 4.5F), 0.0F, 0.0F);
			if(!this.worldObj.isRemote) {
				this.func_41007_az();
			}

			if(!this.worldObj.isRemote && this.maxHurtTime == 0) {
				this.collideWithEntities(this.worldObj.getEntitiesWithinAABBExcludingEntity(this, this.dragonPartWing1.boundingBox.expand(4.0D, 2.0D, 4.0D).offset(0.0D, -2.0D, 0.0D)));
				this.collideWithEntities(this.worldObj.getEntitiesWithinAABBExcludingEntity(this, this.dragonPartWing2.boundingBox.expand(4.0D, 2.0D, 4.0D).offset(0.0D, -2.0D, 0.0D)));
				this.attackEntitiesInList(this.worldObj.getEntitiesWithinAABBExcludingEntity(this, this.dragonPartHead.boundingBox.expand(1.0D, 1.0D, 1.0D)));
			}

			double[] var29 = this.func_40160_a(5, 1.0F);
			double[] var9 = this.func_40160_a(0, 1.0F);
			var31 = MathHelper.sin(this.rotationYaw * (float)Math.PI / 180.0F - this.randomYawVelocity * 0.01F);
			float var33 = MathHelper.cos(this.rotationYaw * (float)Math.PI / 180.0F - this.randomYawVelocity * 0.01F);
			this.dragonPartHead.onUpdate();
			this.dragonPartHead.setLocationAndAngles(this.posX + (double)(var31 * 5.5F * var3), this.posY + (var9[1] - var29[1]) * 1.0D + (double)(var27 * 5.5F), this.posZ - (double)(var33 * 5.5F * var3), 0.0F, 0.0F);

			for(int var30 = 0; var30 < 3; ++var30) {
				EntityDragonPart var32 = null;
				if(var30 == 0) {
					var32 = this.dragonPartTail1;
				}

				if(var30 == 1) {
					var32 = this.dragonPartTail2;
				}

				if(var30 == 2) {
					var32 = this.dragonPartTail3;
				}

				double[] var34 = this.func_40160_a(12 + var30 * 2, 1.0F);
				float var35 = this.rotationYaw * (float)Math.PI / 180.0F + this.simplifyAngle(var34[0] - var29[0]) * (float)Math.PI / 180.0F * 1.0F;
				float var37 = MathHelper.sin(var35);
				float var36 = MathHelper.cos(var35);
				float var38 = 1.5F;
				float var40 = (float)(var30 + 1) * 2.0F;
				var32.onUpdate();
				var32.setLocationAndAngles(this.posX - (double)((var28 * var38 + var37 * var40) * var3), this.posY + (var34[1] - var29[1]) * 1.0D - (double)((var40 + var38) * var27) + 1.5D, this.posZ + (double)((var7 * var38 + var36 * var40) * var3), 0.0F, 0.0F);
			}

			if(!this.worldObj.isRemote) {
				this.field_40161_az = this.destroyBlocksInAABB(this.dragonPartHead.boundingBox) | this.destroyBlocksInAABB(this.dragonPartBody.boundingBox);
			}

		}
	}

	private void updateDragonEnderCrystal() {
		if(this.healingEnderCrystal != null) {
			if(this.healingEnderCrystal.isDead) {
				if(!this.worldObj.isRemote) {
					this.attackEntityFromPart(this.dragonPartHead, DamageSource.explosion, 10);
				}

				this.healingEnderCrystal = null;
			} else if(this.ticksExisted % 10 == 0 && this.health < this.maxHealth) {
				++this.health;
			}
		}

		if(this.rand.nextInt(10) == 0) {
			float var1 = 32.0F;
			List var2 = this.worldObj.getEntitiesWithinAABB(EntityEnderCrystal.class, this.boundingBox.expand((double)var1, (double)var1, (double)var1));
			EntityEnderCrystal var3 = null;
			double var4 = Double.MAX_VALUE;
			Iterator var6 = var2.iterator();

			while(var6.hasNext()) {
				Entity var7 = (Entity)var6.next();
				double var8 = var7.getDistanceSqToEntity(this);
				if(var8 < var4) {
					var4 = var8;
					var3 = (EntityEnderCrystal)var7;
				}
			}

			this.healingEnderCrystal = var3;
		}

	}

	private void func_41007_az() {
	}

	private void collideWithEntities(List var1) {
		double var2 = (this.dragonPartBody.boundingBox.minX + this.dragonPartBody.boundingBox.maxX) / 2.0D;
		double var4 = (this.dragonPartBody.boundingBox.minZ + this.dragonPartBody.boundingBox.maxZ) / 2.0D;
		Iterator var6 = var1.iterator();

		while(var6.hasNext()) {
			Entity var7 = (Entity)var6.next();
			if(var7 instanceof EntityLiving) {
				double var8 = var7.posX - var2;
				double var10 = var7.posZ - var4;
				double var12 = var8 * var8 + var10 * var10;
				var7.addVelocity(var8 / var12 * 4.0D, (double)0.2F, var10 / var12 * 4.0D);
			}
		}

	}

	private void attackEntitiesInList(List var1) {
		for(int var2 = 0; var2 < var1.size(); ++var2) {
			Entity var3 = (Entity)var1.get(var2);
			if(var3 instanceof EntityLiving) {
				var3.attackEntityFrom(DamageSource.causeMobDamage(this), 10);
			}
		}

	}

	private void func_41006_aA() {
		this.field_40163_ay = false;
		if(this.rand.nextInt(2) == 0 && this.worldObj.playerEntities.size() > 0) {
			this.target = (Entity)this.worldObj.playerEntities.get(this.rand.nextInt(this.worldObj.playerEntities.size()));
		} else {
			boolean var1 = false;

			do {
				this.targetX = 0.0D;
				this.targetY = (double)(70.0F + this.rand.nextFloat() * 50.0F);
				this.targetZ = 0.0D;
				this.targetX += (double)(this.rand.nextFloat() * 120.0F - 60.0F);
				this.targetZ += (double)(this.rand.nextFloat() * 120.0F - 60.0F);
				double var2 = this.posX - this.targetX;
				double var4 = this.posY - this.targetY;
				double var6 = this.posZ - this.targetZ;
				var1 = var2 * var2 + var4 * var4 + var6 * var6 > 100.0D;
			} while(!var1);

			this.target = null;
		}

	}

	private float simplifyAngle(double var1) {
		while(var1 >= 180.0D) {
			var1 -= 360.0D;
		}

		while(var1 < -180.0D) {
			var1 += 360.0D;
		}

		return (float)var1;
	}

	private boolean destroyBlocksInAABB(AxisAlignedBB var1) {
		int var2 = MathHelper.floor_double(var1.minX);
		int var3 = MathHelper.floor_double(var1.minY);
		int var4 = MathHelper.floor_double(var1.minZ);
		int var5 = MathHelper.floor_double(var1.maxX);
		int var6 = MathHelper.floor_double(var1.maxY);
		int var7 = MathHelper.floor_double(var1.maxZ);
		boolean var8 = false;
		boolean var9 = false;

		for(int var10 = var2; var10 <= var5; ++var10) {
			for(int var11 = var3; var11 <= var6; ++var11) {
				for(int var12 = var4; var12 <= var7; ++var12) {
					int var13 = this.worldObj.getBlockId(var10, var11, var12);
					if(var13 != 0) {
						if(var13 != Block.obsidian.blockID && var13 != Block.whiteStone.blockID && var13 != Block.bedrock.blockID) {
							var9 = true;
							this.worldObj.setBlockWithNotify(var10, var11, var12, 0);
						} else {
							var8 = true;
						}
					}
				}
			}
		}

		if(var9) {
			double var16 = var1.minX + (var1.maxX - var1.minX) * (double)this.rand.nextFloat();
			double var17 = var1.minY + (var1.maxY - var1.minY) * (double)this.rand.nextFloat();
			double var14 = var1.minZ + (var1.maxZ - var1.minZ) * (double)this.rand.nextFloat();
			this.worldObj.spawnParticle("largeexplode", var16, var17, var14, 0.0D, 0.0D, 0.0D);
		}

		return var8;
	}

	public boolean attackEntityFromPart(EntityDragonPart var1, DamageSource var2, int var3) {
		if(var1 != this.dragonPartHead) {
			var3 = var3 / 4 + 1;
		}

		float var4 = this.rotationYaw * (float)Math.PI / 180.0F;
		float var5 = MathHelper.sin(var4);
		float var6 = MathHelper.cos(var4);
		this.targetX = this.posX + (double)(var5 * 5.0F) + (double)((this.rand.nextFloat() - 0.5F) * 2.0F);
		this.targetY = this.posY + (double)(this.rand.nextFloat() * 3.0F) + 1.0D;
		this.targetZ = this.posZ - (double)(var6 * 5.0F) + (double)((this.rand.nextFloat() - 0.5F) * 2.0F);
		this.target = null;
		if(var2.getEntity() instanceof EntityPlayer || var2 == DamageSource.explosion) {
			this.superAttackFrom(var2, var3);
		}

		return true;
	}

	protected void onDeathUpdate() {
		++this.field_40178_aA;
		if(this.field_40178_aA >= 180 && this.field_40178_aA <= 200) {
			float var1 = (this.rand.nextFloat() - 0.5F) * 8.0F;
			float var2 = (this.rand.nextFloat() - 0.5F) * 4.0F;
			float var3 = (this.rand.nextFloat() - 0.5F) * 8.0F;
			this.worldObj.spawnParticle("hugeexplosion", this.posX + (double)var1, this.posY + 2.0D + (double)var2, this.posZ + (double)var3, 0.0D, 0.0D, 0.0D);
		}

		int var4;
		int var5;
		if(!this.worldObj.isRemote && this.field_40178_aA > 150 && this.field_40178_aA % 5 == 0) {
			var4 = 1000;

			while(var4 > 0) {
				var5 = EntityXPOrb.getXPSplit(var4);
				var4 -= var5;
				this.worldObj.spawnEntityInWorld(new EntityXPOrb(this.worldObj, this.posX, this.posY, this.posZ, var5));
			}
		}

		this.moveEntity(0.0D, (double)0.1F, 0.0D);
		this.renderYawOffset = this.rotationYaw += 20.0F;
		if(this.field_40178_aA == 200) {
			var4 = 10000;

			while(var4 > 0) {
				var5 = EntityXPOrb.getXPSplit(var4);
				var4 -= var5;
				this.worldObj.spawnEntityInWorld(new EntityXPOrb(this.worldObj, this.posX, this.posY, this.posZ, var5));
			}

			this.createEnderPortal(MathHelper.floor_double(this.posX), MathHelper.floor_double(this.posZ));
			this.onEntityDeath();
			this.setDead();
		}

	}

	private void createEnderPortal(int var1, int var2) {
		byte var3 = 64;
		BlockEndPortal.bossDefeated = true;
		byte var4 = 4;

		for(int var5 = var3 - 1; var5 <= var3 + 32; ++var5) {
			for(int var6 = var1 - var4; var6 <= var1 + var4; ++var6) {
				for(int var7 = var2 - var4; var7 <= var2 + var4; ++var7) {
					double var8 = (double)(var6 - var1);
					double var10 = (double)(var7 - var2);
					double var12 = (double)MathHelper.sqrt_double(var8 * var8 + var10 * var10);
					if(var12 <= (double)var4 - 0.5D) {
						if(var5 < var3) {
							if(var12 <= (double)(var4 - 1) - 0.5D) {
								this.worldObj.setBlockWithNotify(var6, var5, var7, Block.bedrock.blockID);
							}
						} else if(var5 > var3) {
							this.worldObj.setBlockWithNotify(var6, var5, var7, 0);
						} else if(var12 > (double)(var4 - 1) - 0.5D) {
							this.worldObj.setBlockWithNotify(var6, var5, var7, Block.bedrock.blockID);
						} else {
							this.worldObj.setBlockWithNotify(var6, var5, var7, Block.endPortal.blockID);
						}
					}
				}
			}
		}

		this.worldObj.setBlockWithNotify(var1, var3 + 0, var2, Block.bedrock.blockID);
		this.worldObj.setBlockWithNotify(var1, var3 + 1, var2, Block.bedrock.blockID);
		this.worldObj.setBlockWithNotify(var1, var3 + 2, var2, Block.bedrock.blockID);
		this.worldObj.setBlockWithNotify(var1 - 1, var3 + 2, var2, Block.torchWood.blockID);
		this.worldObj.setBlockWithNotify(var1 + 1, var3 + 2, var2, Block.torchWood.blockID);
		this.worldObj.setBlockWithNotify(var1, var3 + 2, var2 - 1, Block.torchWood.blockID);
		this.worldObj.setBlockWithNotify(var1, var3 + 2, var2 + 1, Block.torchWood.blockID);
		this.worldObj.setBlockWithNotify(var1, var3 + 3, var2, Block.bedrock.blockID);
		this.worldObj.setBlockWithNotify(var1, var3 + 4, var2, Block.dragonEgg.blockID);
		BlockEndPortal.bossDefeated = false;
	}

	protected void despawnEntity() {
	}

	public Entity[] getParts() {
		return this.dragonPartArray;
	}

	public boolean canBeCollidedWith() {
		return false;
	}

	public int func_41010_ax() {
		return this.dataWatcher.getWatchableObjectInt(16);
	}
}
