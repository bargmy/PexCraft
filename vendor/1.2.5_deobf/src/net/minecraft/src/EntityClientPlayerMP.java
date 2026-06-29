package net.minecraft.src;

import net.minecraft.client.Minecraft;

public class EntityClientPlayerMP extends EntityPlayerSP {
	public NetClientHandler sendQueue;
	private int inventoryUpdateTickCounter = 0;
	private double oldPosX;
	private double oldMinY;
	private double oldPosY;
	private double oldPosZ;
	private float oldRotationYaw;
	private float oldRotationPitch;
	private boolean wasOnGround = false;
	private boolean shouldStopSneaking = false;
	private boolean wasSneaking = false;
	private int timeSinceMoved = 0;
	private boolean hasSetHealth = false;

	public EntityClientPlayerMP(Minecraft var1, World var2, Session var3, NetClientHandler var4) {
		super(var1, var2, var3, 0);
		this.sendQueue = var4;
	}

	public boolean attackEntityFrom(DamageSource var1, int var2) {
		return false;
	}

	public void heal(int var1) {
	}

	public void onUpdate() {
		if(this.worldObj.blockExists(MathHelper.floor_double(this.posX), 0, MathHelper.floor_double(this.posZ))) {
			super.onUpdate();
			this.sendMotionUpdates();
		}
	}

	public void sendMotionUpdates() {
		if(this.inventoryUpdateTickCounter++ == 20) {
			this.inventoryUpdateTickCounter = 0;
		}

		boolean var1 = this.isSprinting();
		if(var1 != this.wasSneaking) {
			if(var1) {
				this.sendQueue.addToSendQueue(new Packet19EntityAction(this, 4));
			} else {
				this.sendQueue.addToSendQueue(new Packet19EntityAction(this, 5));
			}

			this.wasSneaking = var1;
		}

		boolean var2 = this.isSneaking();
		if(var2 != this.shouldStopSneaking) {
			if(var2) {
				this.sendQueue.addToSendQueue(new Packet19EntityAction(this, 1));
			} else {
				this.sendQueue.addToSendQueue(new Packet19EntityAction(this, 2));
			}

			this.shouldStopSneaking = var2;
		}

		double var3 = this.posX - this.oldPosX;
		double var5 = this.boundingBox.minY - this.oldMinY;
		double var7 = this.posY - this.oldPosY;
		double var9 = this.posZ - this.oldPosZ;
		double var11 = (double)(this.rotationYaw - this.oldRotationYaw);
		double var13 = (double)(this.rotationPitch - this.oldRotationPitch);
		boolean var15 = var5 != 0.0D || var7 != 0.0D || var3 != 0.0D || var9 != 0.0D;
		boolean var16 = var11 != 0.0D || var13 != 0.0D;
		if(this.ridingEntity != null) {
			if(var16) {
				this.sendQueue.addToSendQueue(new Packet11PlayerPosition(this.motionX, -999.0D, -999.0D, this.motionZ, this.onGround));
			} else {
				this.sendQueue.addToSendQueue(new Packet13PlayerLookMove(this.motionX, -999.0D, -999.0D, this.motionZ, this.rotationYaw, this.rotationPitch, this.onGround));
			}

			var15 = false;
		} else if(var15 && var16) {
			this.sendQueue.addToSendQueue(new Packet13PlayerLookMove(this.posX, this.boundingBox.minY, this.posY, this.posZ, this.rotationYaw, this.rotationPitch, this.onGround));
			this.timeSinceMoved = 0;
		} else if(var15) {
			this.sendQueue.addToSendQueue(new Packet11PlayerPosition(this.posX, this.boundingBox.minY, this.posY, this.posZ, this.onGround));
			this.timeSinceMoved = 0;
		} else if(var16) {
			this.sendQueue.addToSendQueue(new Packet12PlayerLook(this.rotationYaw, this.rotationPitch, this.onGround));
			this.timeSinceMoved = 0;
		} else {
			this.sendQueue.addToSendQueue(new Packet10Flying(this.onGround));
			if(this.wasOnGround == this.onGround && this.timeSinceMoved <= 200) {
				++this.timeSinceMoved;
			} else {
				this.timeSinceMoved = 0;
			}
		}

		this.wasOnGround = this.onGround;
		if(var15) {
			this.oldPosX = this.posX;
			this.oldMinY = this.boundingBox.minY;
			this.oldPosY = this.posY;
			this.oldPosZ = this.posZ;
		}

		if(var16) {
			this.oldRotationYaw = this.rotationYaw;
			this.oldRotationPitch = this.rotationPitch;
		}

	}

	public EntityItem dropOneItem() {
		this.sendQueue.addToSendQueue(new Packet14BlockDig(4, 0, 0, 0, 0));
		return null;
	}

	protected void joinEntityItemWithWorld(EntityItem var1) {
	}

	public void sendChatMessage(String var1) {
		if(this.mc.ingameGUI.func_50013_c().size() == 0 || !((String)this.mc.ingameGUI.func_50013_c().get(this.mc.ingameGUI.func_50013_c().size() - 1)).equals(var1)) {
			this.mc.ingameGUI.func_50013_c().add(var1);
		}

		this.sendQueue.addToSendQueue(new Packet3Chat(var1));
	}

	public void swingItem() {
		super.swingItem();
		this.sendQueue.addToSendQueue(new Packet18Animation(this, 1));
	}

	public void respawnPlayer() {
		this.sendQueue.addToSendQueue(new Packet9Respawn(this.dimension, (byte)this.worldObj.difficultySetting, this.worldObj.getWorldInfo().getTerrainType(), this.worldObj.getHeight(), 0));
	}

	protected void damageEntity(DamageSource var1, int var2) {
		this.setEntityHealth(this.getHealth() - var2);
	}

	public void closeScreen() {
		this.sendQueue.addToSendQueue(new Packet101CloseWindow(this.craftingInventory.windowId));
		this.inventory.setItemStack((ItemStack)null);
		super.closeScreen();
	}

	public void setHealth(int var1) {
		if(this.hasSetHealth) {
			super.setHealth(var1);
		} else {
			this.setEntityHealth(var1);
			this.hasSetHealth = true;
		}

	}

	public void addStat(StatBase var1, int var2) {
		if(var1 != null) {
			if(var1.isIndependent) {
				super.addStat(var1, var2);
			}

		}
	}

	public void incrementStat(StatBase var1, int var2) {
		if(var1 != null) {
			if(!var1.isIndependent) {
				super.addStat(var1, var2);
			}

		}
	}

	public void func_50009_aI() {
		this.sendQueue.addToSendQueue(new Packet202PlayerAbilities(this.capabilities));
	}
}
