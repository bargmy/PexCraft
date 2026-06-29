package net.minecraft.src;

public class EntityVillager extends EntityAgeable {
	private int randomTickDivider;
	private boolean isMatingFlag;
	private boolean isPlayingFlag;
	Village villageObj;

	public EntityVillager(World var1) {
		this(var1, 0);
	}

	public EntityVillager(World var1, int var2) {
		super(var1);
		this.randomTickDivider = 0;
		this.isMatingFlag = false;
		this.isPlayingFlag = false;
		this.villageObj = null;
		this.setProfession(var2);
		this.texture = "/mob/villager/villager.png";
		this.moveSpeed = 0.5F;
		this.getNavigator().setBreakDoors(true);
		this.getNavigator().func_48664_a(true);
		this.tasks.addTask(0, new EntityAISwimming(this));
		this.tasks.addTask(1, new EntityAIAvoidEntity(this, EntityZombie.class, 8.0F, 0.3F, 0.35F));
		this.tasks.addTask(2, new EntityAIMoveIndoors(this));
		this.tasks.addTask(3, new EntityAIRestrictOpenDoor(this));
		this.tasks.addTask(4, new EntityAIOpenDoor(this, true));
		this.tasks.addTask(5, new EntityAIMoveTwardsRestriction(this, 0.3F));
		this.tasks.addTask(6, new EntityAIVillagerMate(this));
		this.tasks.addTask(7, new EntityAIFollowGolem(this));
		this.tasks.addTask(8, new EntityAIPlay(this, 0.32F));
		this.tasks.addTask(9, new EntityAIWatchClosest2(this, EntityPlayer.class, 3.0F, 1.0F));
		this.tasks.addTask(9, new EntityAIWatchClosest2(this, EntityVillager.class, 5.0F, 0.02F));
		this.tasks.addTask(9, new EntityAIWander(this, 0.3F));
		this.tasks.addTask(10, new EntityAIWatchClosest(this, EntityLiving.class, 8.0F));
	}

	public boolean isAIEnabled() {
		return true;
	}

	protected void updateAITick() {
		if(--this.randomTickDivider <= 0) {
			this.worldObj.villageCollectionObj.addVillagerPosition(MathHelper.floor_double(this.posX), MathHelper.floor_double(this.posY), MathHelper.floor_double(this.posZ));
			this.randomTickDivider = 70 + this.rand.nextInt(50);
			this.villageObj = this.worldObj.villageCollectionObj.findNearestVillage(MathHelper.floor_double(this.posX), MathHelper.floor_double(this.posY), MathHelper.floor_double(this.posZ), 32);
			if(this.villageObj == null) {
				this.detachHome();
			} else {
				ChunkCoordinates var1 = this.villageObj.getCenter();
				this.setHomeArea(var1.posX, var1.posY, var1.posZ, this.villageObj.getVillageRadius());
			}
		}

		super.updateAITick();
	}

	protected void entityInit() {
		super.entityInit();
		this.dataWatcher.addObject(16, Integer.valueOf(0));
	}

	public int getMaxHealth() {
		return 20;
	}

	public void onLivingUpdate() {
		super.onLivingUpdate();
	}

	public void writeEntityToNBT(NBTTagCompound var1) {
		super.writeEntityToNBT(var1);
		var1.setInteger("Profession", this.getProfession());
	}

	public void readEntityFromNBT(NBTTagCompound var1) {
		super.readEntityFromNBT(var1);
		this.setProfession(var1.getInteger("Profession"));
	}

	public String getTexture() {
		switch(this.getProfession()) {
		case 0:
			return "/mob/villager/farmer.png";
		case 1:
			return "/mob/villager/librarian.png";
		case 2:
			return "/mob/villager/priest.png";
		case 3:
			return "/mob/villager/smith.png";
		case 4:
			return "/mob/villager/butcher.png";
		default:
			return super.getTexture();
		}
	}

	protected boolean canDespawn() {
		return false;
	}

	protected String getLivingSound() {
		return "mob.villager.default";
	}

	protected String getHurtSound() {
		return "mob.villager.defaulthurt";
	}

	protected String getDeathSound() {
		return "mob.villager.defaultdeath";
	}

	public void setProfession(int var1) {
		this.dataWatcher.updateObject(16, Integer.valueOf(var1));
	}

	public int getProfession() {
		return this.dataWatcher.getWatchableObjectInt(16);
	}

	public boolean getIsMatingFlag() {
		return this.isMatingFlag;
	}

	public void setIsMatingFlag(boolean var1) {
		this.isMatingFlag = var1;
	}

	public void setIsPlayingFlag(boolean var1) {
		this.isPlayingFlag = var1;
	}

	public boolean getIsPlayingFlag() {
		return this.isPlayingFlag;
	}

	public void setRevengeTarget(EntityLiving var1) {
		super.setRevengeTarget(var1);
		if(this.villageObj != null && var1 != null) {
			this.villageObj.addOrRenewAgressor(var1);
		}

	}
}
