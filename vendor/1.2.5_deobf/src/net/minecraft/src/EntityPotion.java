package net.minecraft.src;

import java.util.Iterator;
import java.util.List;

public class EntityPotion extends EntityThrowable {
	private int potionDamage;

	public EntityPotion(World var1) {
		super(var1);
	}

	public EntityPotion(World var1, EntityLiving var2, int var3) {
		super(var1, var2);
		this.potionDamage = var3;
	}

	public EntityPotion(World var1, double var2, double var4, double var6, int var8) {
		super(var1, var2, var4, var6);
		this.potionDamage = var8;
	}

	protected float func_40075_e() {
		return 0.05F;
	}

	protected float func_40077_c() {
		return 0.5F;
	}

	protected float func_40074_d() {
		return -20.0F;
	}

	public int getPotionDamage() {
		return this.potionDamage;
	}

	protected void onImpact(MovingObjectPosition var1) {
		if(!this.worldObj.isRemote) {
			List var2 = Item.potion.getEffects(this.potionDamage);
			if(var2 != null && !var2.isEmpty()) {
				AxisAlignedBB var3 = this.boundingBox.expand(4.0D, 2.0D, 4.0D);
				List var4 = this.worldObj.getEntitiesWithinAABB(EntityLiving.class, var3);
				if(var4 != null && !var4.isEmpty()) {
					Iterator var5 = var4.iterator();

					label48:
					while(true) {
						Entity var6;
						double var7;
						do {
							if(!var5.hasNext()) {
								break label48;
							}

							var6 = (Entity)var5.next();
							var7 = this.getDistanceSqToEntity(var6);
						} while(var7 >= 16.0D);

						double var9 = 1.0D - Math.sqrt(var7) / 4.0D;
						if(var6 == var1.entityHit) {
							var9 = 1.0D;
						}

						Iterator var11 = var2.iterator();

						while(var11.hasNext()) {
							PotionEffect var12 = (PotionEffect)var11.next();
							int var13 = var12.getPotionID();
							if(Potion.potionTypes[var13].isInstant()) {
								Potion.potionTypes[var13].affectEntity(this.thrower, (EntityLiving)var6, var12.getAmplifier(), var9);
							} else {
								int var14 = (int)(var9 * (double)var12.getDuration() + 0.5D);
								if(var14 > 20) {
									((EntityLiving)var6).addPotionEffect(new PotionEffect(var13, var14, var12.getAmplifier()));
								}
							}
						}
					}
				}
			}

			this.worldObj.playAuxSFX(2002, (int)Math.round(this.posX), (int)Math.round(this.posY), (int)Math.round(this.posZ), this.potionDamage);
			this.setDead();
		}

	}
}
