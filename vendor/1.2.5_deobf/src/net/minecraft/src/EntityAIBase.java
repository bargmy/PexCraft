package net.minecraft.src;

public abstract class EntityAIBase {
	private int mutexBits = 0;

	public abstract boolean shouldExecute();

	public boolean continueExecuting() {
		return this.shouldExecute();
	}

	public boolean isContinuous() {
		return true;
	}

	public void startExecuting() {
	}

	public void resetTask() {
	}

	public void updateTask() {
	}

	public void setMutexBits(int var1) {
		this.mutexBits = var1;
	}

	public int getMutexBits() {
		return this.mutexBits;
	}
}
