package net.minecraft.src;

public abstract class EntityAgeable extends EntityCreature {
	public EntityAgeable(World var1) {
		super(var1);
	}

	protected void entityInit() {
		super.entityInit();
		this.dataWatcher.addObject(12, new Integer(0));
	}

	public int getGrowingAge() {
		return this.dataWatcher.getWatchableObjectInt(12);
	}

	public void setGrowingAge(int var1) {
		this.dataWatcher.updateObject(12, Integer.valueOf(var1));
	}

	public void writeEntityToNBT(NBTTagCompound var1) {
		super.writeEntityToNBT(var1);
		var1.setInteger("Age", this.getGrowingAge());
	}

	public void readEntityFromNBT(NBTTagCompound var1) {
		super.readEntityFromNBT(var1);
		this.setGrowingAge(var1.getInteger("Age"));
	}

	public void onLivingUpdate() {
		super.onLivingUpdate();
		int var1 = this.getGrowingAge();
		if(var1 < 0) {
			++var1;
			this.setGrowingAge(var1);
		} else if(var1 > 0) {
			--var1;
			this.setGrowingAge(var1);
		}

	}

	public boolean isChild() {
		return this.getGrowingAge() < 0;
	}
}
