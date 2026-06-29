package net.minecraft.src;

public class EntityAIBeg extends EntityAIBase {
	private EntityWolf theWolf;
	private EntityPlayer field_48348_b;
	private World field_48349_c;
	private float field_48346_d;
	private int field_48347_e;

	public EntityAIBeg(EntityWolf var1, float var2) {
		this.theWolf = var1;
		this.field_48349_c = var1.worldObj;
		this.field_48346_d = var2;
		this.setMutexBits(2);
	}

	public boolean shouldExecute() {
		this.field_48348_b = this.field_48349_c.getClosestPlayerToEntity(this.theWolf, (double)this.field_48346_d);
		return this.field_48348_b == null ? false : this.func_48345_a(this.field_48348_b);
	}

	public boolean continueExecuting() {
		return !this.field_48348_b.isEntityAlive() ? false : (this.theWolf.getDistanceSqToEntity(this.field_48348_b) > (double)(this.field_48346_d * this.field_48346_d) ? false : this.field_48347_e > 0 && this.func_48345_a(this.field_48348_b));
	}

	public void startExecuting() {
		this.theWolf.func_48150_h(true);
		this.field_48347_e = 40 + this.theWolf.getRNG().nextInt(40);
	}

	public void resetTask() {
		this.theWolf.func_48150_h(false);
		this.field_48348_b = null;
	}

	public void updateTask() {
		this.theWolf.getLookHelper().setLookPosition(this.field_48348_b.posX, this.field_48348_b.posY + (double)this.field_48348_b.getEyeHeight(), this.field_48348_b.posZ, 10.0F, (float)this.theWolf.getVerticalFaceSpeed());
		--this.field_48347_e;
	}

	private boolean func_48345_a(EntityPlayer var1) {
		ItemStack var2 = var1.inventory.getCurrentItem();
		return var2 == null ? false : (!this.theWolf.isTamed() && var2.itemID == Item.bone.shiftedIndex ? true : this.theWolf.isWheat(var2));
	}
}
