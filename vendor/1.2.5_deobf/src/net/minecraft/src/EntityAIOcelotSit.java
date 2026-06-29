package net.minecraft.src;

public class EntityAIOcelotSit extends EntityAIBase {
	private final EntityOcelot field_50085_a;
	private final float field_50083_b;
	private int field_50084_c = 0;
	private int field_52011_h = 0;
	private int field_50081_d = 0;
	private int field_50082_e = 0;
	private int field_50079_f = 0;
	private int field_50080_g = 0;

	public EntityAIOcelotSit(EntityOcelot var1, float var2) {
		this.field_50085_a = var1;
		this.field_50083_b = var2;
		this.setMutexBits(5);
	}

	public boolean shouldExecute() {
		return this.field_50085_a.isTamed() && !this.field_50085_a.isSitting() && this.field_50085_a.getRNG().nextDouble() <= (double)0.0065F && this.func_50077_h();
	}

	public boolean continueExecuting() {
		return this.field_50084_c <= this.field_50081_d && this.field_52011_h <= 60 && this.func_50078_a(this.field_50085_a.worldObj, this.field_50082_e, this.field_50079_f, this.field_50080_g);
	}

	public void startExecuting() {
		this.field_50085_a.getNavigator().func_48666_a((double)((float)this.field_50082_e) + 0.5D, (double)(this.field_50079_f + 1), (double)((float)this.field_50080_g) + 0.5D, this.field_50083_b);
		this.field_50084_c = 0;
		this.field_52011_h = 0;
		this.field_50081_d = this.field_50085_a.getRNG().nextInt(this.field_50085_a.getRNG().nextInt(1200) + 1200) + 1200;
		this.field_50085_a.func_50008_ai().func_48407_a(false);
	}

	public void resetTask() {
		this.field_50085_a.func_48140_f(false);
	}

	public void updateTask() {
		++this.field_50084_c;
		this.field_50085_a.func_50008_ai().func_48407_a(false);
		if(this.field_50085_a.getDistanceSq((double)this.field_50082_e, (double)(this.field_50079_f + 1), (double)this.field_50080_g) > 1.0D) {
			this.field_50085_a.func_48140_f(false);
			this.field_50085_a.getNavigator().func_48666_a((double)((float)this.field_50082_e) + 0.5D, (double)(this.field_50079_f + 1), (double)((float)this.field_50080_g) + 0.5D, this.field_50083_b);
			++this.field_52011_h;
		} else if(!this.field_50085_a.isSitting()) {
			this.field_50085_a.func_48140_f(true);
		} else {
			--this.field_52011_h;
		}

	}

	private boolean func_50077_h() {
		int var1 = (int)this.field_50085_a.posY;
		double var2 = (double)Integer.MAX_VALUE;

		for(int var4 = (int)this.field_50085_a.posX - 8; (double)var4 < this.field_50085_a.posX + 8.0D; ++var4) {
			for(int var5 = (int)this.field_50085_a.posZ - 8; (double)var5 < this.field_50085_a.posZ + 8.0D; ++var5) {
				if(this.func_50078_a(this.field_50085_a.worldObj, var4, var1, var5) && this.field_50085_a.worldObj.isAirBlock(var4, var1 + 1, var5)) {
					double var6 = this.field_50085_a.getDistanceSq((double)var4, (double)var1, (double)var5);
					if(var6 < var2) {
						this.field_50082_e = var4;
						this.field_50079_f = var1;
						this.field_50080_g = var5;
						var2 = var6;
					}
				}
			}
		}

		return var2 < (double)Integer.MAX_VALUE;
	}

	private boolean func_50078_a(World var1, int var2, int var3, int var4) {
		int var5 = var1.getBlockId(var2, var3, var4);
		int var6 = var1.getBlockMetadata(var2, var3, var4);
		if(var5 == Block.chest.blockID) {
			TileEntityChest var7 = (TileEntityChest)var1.getBlockTileEntity(var2, var3, var4);
			if(var7.numUsingPlayers < 1) {
				return true;
			}
		} else {
			if(var5 == Block.stoneOvenActive.blockID) {
				return true;
			}

			if(var5 == Block.bed.blockID && !BlockBed.isBlockFootOfBed(var6)) {
				return true;
			}
		}

		return false;
	}
}
