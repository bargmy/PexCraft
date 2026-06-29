package net.minecraft.src;

import org.lwjgl.opengl.GL11;

public class RenderBiped extends RenderLiving {
	protected ModelBiped modelBipedMain;
	protected float field_40296_d;

	public RenderBiped(ModelBiped var1, float var2) {
		this(var1, var2, 1.0F);
		this.modelBipedMain = var1;
	}

	public RenderBiped(ModelBiped var1, float var2, float var3) {
		super(var1, var2);
		this.modelBipedMain = var1;
		this.field_40296_d = var3;
	}

	protected void renderEquippedItems(EntityLiving var1, float var2) {
		super.renderEquippedItems(var1, var2);
		ItemStack var3 = var1.getHeldItem();
		if(var3 != null) {
			GL11.glPushMatrix();
			this.modelBipedMain.bipedRightArm.postRender(1.0F / 16.0F);
			GL11.glTranslatef(-(1.0F / 16.0F), 7.0F / 16.0F, 1.0F / 16.0F);
			float var4;
			if(var3.itemID < 256 && RenderBlocks.renderItemIn3d(Block.blocksList[var3.itemID].getRenderType())) {
				var4 = 0.5F;
				GL11.glTranslatef(0.0F, 3.0F / 16.0F, -(5.0F / 16.0F));
				var4 *= 12.0F / 16.0F;
				GL11.glRotatef(20.0F, 1.0F, 0.0F, 0.0F);
				GL11.glRotatef(45.0F, 0.0F, 1.0F, 0.0F);
				GL11.glScalef(var4, -var4, var4);
			} else if(var3.itemID == Item.bow.shiftedIndex) {
				var4 = 10.0F / 16.0F;
				GL11.glTranslatef(0.0F, 2.0F / 16.0F, 5.0F / 16.0F);
				GL11.glRotatef(-20.0F, 0.0F, 1.0F, 0.0F);
				GL11.glScalef(var4, -var4, var4);
				GL11.glRotatef(-100.0F, 1.0F, 0.0F, 0.0F);
				GL11.glRotatef(45.0F, 0.0F, 1.0F, 0.0F);
			} else if(Item.itemsList[var3.itemID].isFull3D()) {
				var4 = 10.0F / 16.0F;
				GL11.glTranslatef(0.0F, 3.0F / 16.0F, 0.0F);
				GL11.glScalef(var4, -var4, var4);
				GL11.glRotatef(-100.0F, 1.0F, 0.0F, 0.0F);
				GL11.glRotatef(45.0F, 0.0F, 1.0F, 0.0F);
			} else {
				var4 = 6.0F / 16.0F;
				GL11.glTranslatef(0.25F, 3.0F / 16.0F, -(3.0F / 16.0F));
				GL11.glScalef(var4, var4, var4);
				GL11.glRotatef(60.0F, 0.0F, 0.0F, 1.0F);
				GL11.glRotatef(-90.0F, 1.0F, 0.0F, 0.0F);
				GL11.glRotatef(20.0F, 0.0F, 0.0F, 1.0F);
			}

			this.renderManager.itemRenderer.renderItem(var1, var3, 0);
			if(var3.getItem().func_46058_c()) {
				this.renderManager.itemRenderer.renderItem(var1, var3, 1);
			}

			GL11.glPopMatrix();
		}

	}
}
