package net.minecraft.src;

import org.lwjgl.opengl.GL11;

public class RenderEnchantmentTable extends TileEntitySpecialRenderer {
	private ModelBook field_40450_a = new ModelBook();

	public void func_40449_a(TileEntityEnchantmentTable var1, double var2, double var4, double var6, float var8) {
		GL11.glPushMatrix();
		GL11.glTranslatef((float)var2 + 0.5F, (float)var4 + 12.0F / 16.0F, (float)var6 + 0.5F);
		float var9 = (float)var1.tickCount + var8;
		GL11.glTranslatef(0.0F, 0.1F + MathHelper.sin(var9 * 0.1F) * 0.01F, 0.0F);

		float var10;
		for(var10 = var1.bookRotation2 - var1.bookRotationPrev; var10 >= (float)Math.PI; var10 -= (float)Math.PI * 2.0F) {
		}

		while(var10 < -((float)Math.PI)) {
			var10 += (float)Math.PI * 2.0F;
		}

		float var11 = var1.bookRotationPrev + var10 * var8;
		GL11.glRotatef(-var11 * 180.0F / (float)Math.PI, 0.0F, 1.0F, 0.0F);
		GL11.glRotatef(80.0F, 0.0F, 0.0F, 1.0F);
		this.bindTextureByName("/item/book.png");
		float var12 = var1.pageFlipPrev + (var1.pageFlip - var1.pageFlipPrev) * var8 + 0.25F;
		float var13 = var1.pageFlipPrev + (var1.pageFlip - var1.pageFlipPrev) * var8 + 12.0F / 16.0F;
		var12 = (var12 - (float)MathHelper.func_40346_b((double)var12)) * 1.6F - 0.3F;
		var13 = (var13 - (float)MathHelper.func_40346_b((double)var13)) * 1.6F - 0.3F;
		if(var12 < 0.0F) {
			var12 = 0.0F;
		}

		if(var13 < 0.0F) {
			var13 = 0.0F;
		}

		if(var12 > 1.0F) {
			var12 = 1.0F;
		}

		if(var13 > 1.0F) {
			var13 = 1.0F;
		}

		float var14 = var1.bookSpreadPrev + (var1.bookSpread - var1.bookSpreadPrev) * var8;
		this.field_40450_a.render((Entity)null, var9, var12, var13, var14, 0.0F, 1.0F / 16.0F);
		GL11.glPopMatrix();
	}

	public void renderTileEntityAt(TileEntity var1, double var2, double var4, double var6, float var8) {
		this.func_40449_a((TileEntityEnchantmentTable)var1, var2, var4, var6, var8);
	}
}
