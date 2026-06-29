package net.minecraft.src;

import java.util.Collections;
import java.util.Iterator;
import java.util.List;

public class EntityAINearestAttackableTarget extends EntityAITarget {
	EntityLiving targetEntity;
	Class targetClass;
	int field_48386_f;
	private EntityAINearestAttackableTargetSorter field_48387_g;

	public EntityAINearestAttackableTarget(EntityLiving var1, Class var2, float var3, int var4, boolean var5) {
		this(var1, var2, var3, var4, var5, false);
	}

	public EntityAINearestAttackableTarget(EntityLiving var1, Class var2, float var3, int var4, boolean var5, boolean var6) {
		super(var1, var3, var5, var6);
		this.targetClass = var2;
		this.field_48379_d = var3;
		this.field_48386_f = var4;
		this.field_48387_g = new EntityAINearestAttackableTargetSorter(this, var1);
		this.setMutexBits(1);
	}

	public boolean shouldExecute() {
		if(this.field_48386_f > 0 && this.taskOwner.getRNG().nextInt(this.field_48386_f) != 0) {
			return false;
		} else {
			if(this.targetClass == EntityPlayer.class) {
				EntityPlayer var1 = this.taskOwner.worldObj.getClosestVulnerablePlayerToEntity(this.taskOwner, (double)this.field_48379_d);
				if(this.func_48376_a(var1, false)) {
					this.targetEntity = var1;
					return true;
				}
			} else {
				List var5 = this.taskOwner.worldObj.getEntitiesWithinAABB(this.targetClass, this.taskOwner.boundingBox.expand((double)this.field_48379_d, 4.0D, (double)this.field_48379_d));
				Collections.sort(var5, this.field_48387_g);
				Iterator var2 = var5.iterator();

				while(var2.hasNext()) {
					Entity var3 = (Entity)var2.next();
					EntityLiving var4 = (EntityLiving)var3;
					if(this.func_48376_a(var4, false)) {
						this.targetEntity = var4;
						return true;
					}
				}
			}

			return false;
		}
	}

	public void startExecuting() {
		this.taskOwner.setAttackTarget(this.targetEntity);
		super.startExecuting();
	}
}
