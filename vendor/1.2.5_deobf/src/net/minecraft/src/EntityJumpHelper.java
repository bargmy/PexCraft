package net.minecraft.src;

public class EntityJumpHelper {
	private EntityLiving entity;
	private boolean isJumping = false;

	public EntityJumpHelper(EntityLiving var1) {
		this.entity = var1;
	}

	public void setJumping() {
		this.isJumping = true;
	}

	public void doJump() {
		this.entity.setJumping(this.isJumping);
		this.isJumping = false;
	}
}
