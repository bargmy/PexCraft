package net.minecraft.src;

import java.util.Comparator;

public class RenderSorter implements Comparator {
	private EntityLiving baseEntity;

	public RenderSorter(EntityLiving var1) {
		this.baseEntity = var1;
	}

	public int doCompare(WorldRenderer var1, WorldRenderer var2) {
		if(var1.isInFrustum && !var2.isInFrustum) {
			return 1;
		} else if(var2.isInFrustum && !var1.isInFrustum) {
			return -1;
		} else {
			double var3 = (double)var1.distanceToEntitySquared(this.baseEntity);
			double var5 = (double)var2.distanceToEntitySquared(this.baseEntity);
			return var3 < var5 ? 1 : (var3 > var5 ? -1 : (var1.chunkIndex < var2.chunkIndex ? 1 : -1));
		}
	}

	public int compare(Object var1, Object var2) {
		return this.doCompare((WorldRenderer)var1, (WorldRenderer)var2);
	}
}
