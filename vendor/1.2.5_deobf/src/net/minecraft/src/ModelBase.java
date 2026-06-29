package net.minecraft.src;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public abstract class ModelBase {
	public float onGround;
	public boolean isRiding = false;
	public List boxList = new ArrayList();
	public boolean isChild = true;
	private Map modelTextureMap = new HashMap();
	public int textureWidth = 64;
	public int textureHeight = 32;

	public void render(Entity var1, float var2, float var3, float var4, float var5, float var6, float var7) {
	}

	public void setRotationAngles(float var1, float var2, float var3, float var4, float var5, float var6) {
	}

	public void setLivingAnimations(EntityLiving var1, float var2, float var3, float var4) {
	}

	protected void setTextureOffset(String var1, int var2, int var3) {
		this.modelTextureMap.put(var1, new TextureOffset(var2, var3));
	}

	public TextureOffset getTextureOffset(String var1) {
		return (TextureOffset)this.modelTextureMap.get(var1);
	}
}
