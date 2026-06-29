package net.minecraft.src;

import java.util.ArrayList;
import java.util.List;
import org.lwjgl.opengl.GL11;

public class ModelRenderer {
	public float textureWidth;
	public float textureHeight;
	private int textureOffsetX;
	private int textureOffsetY;
	public float rotationPointX;
	public float rotationPointY;
	public float rotationPointZ;
	public float rotateAngleX;
	public float rotateAngleY;
	public float rotateAngleZ;
	private boolean compiled;
	private int displayList;
	public boolean mirror;
	public boolean showModel;
	public boolean isHidden;
	public List cubeList;
	public List childModels;
	public final String boxName;
	private ModelBase baseModel;

	public ModelRenderer(ModelBase var1, String var2) {
		this.textureWidth = 64.0F;
		this.textureHeight = 32.0F;
		this.compiled = false;
		this.displayList = 0;
		this.mirror = false;
		this.showModel = true;
		this.isHidden = false;
		this.cubeList = new ArrayList();
		this.baseModel = var1;
		var1.boxList.add(this);
		this.boxName = var2;
		this.setTextureSize(var1.textureWidth, var1.textureHeight);
	}

	public ModelRenderer(ModelBase var1) {
		this(var1, (String)null);
	}

	public ModelRenderer(ModelBase var1, int var2, int var3) {
		this(var1);
		this.setTextureOffset(var2, var3);
	}

	public void addChild(ModelRenderer var1) {
		if(this.childModels == null) {
			this.childModels = new ArrayList();
		}

		this.childModels.add(var1);
	}

	public ModelRenderer setTextureOffset(int var1, int var2) {
		this.textureOffsetX = var1;
		this.textureOffsetY = var2;
		return this;
	}

	public ModelRenderer addBox(String var1, float var2, float var3, float var4, int var5, int var6, int var7) {
		var1 = this.boxName + "." + var1;
		TextureOffset var8 = this.baseModel.getTextureOffset(var1);
		this.setTextureOffset(var8.field_40734_a, var8.field_40733_b);
		this.cubeList.add((new ModelBox(this, this.textureOffsetX, this.textureOffsetY, var2, var3, var4, var5, var6, var7, 0.0F)).func_40671_a(var1));
		return this;
	}

	public ModelRenderer addBox(float var1, float var2, float var3, int var4, int var5, int var6) {
		this.cubeList.add(new ModelBox(this, this.textureOffsetX, this.textureOffsetY, var1, var2, var3, var4, var5, var6, 0.0F));
		return this;
	}

	public void addBox(float var1, float var2, float var3, int var4, int var5, int var6, float var7) {
		this.cubeList.add(new ModelBox(this, this.textureOffsetX, this.textureOffsetY, var1, var2, var3, var4, var5, var6, var7));
	}

	public void setRotationPoint(float var1, float var2, float var3) {
		this.rotationPointX = var1;
		this.rotationPointY = var2;
		this.rotationPointZ = var3;
	}

	public void render(float var1) {
		if(!this.isHidden) {
			if(this.showModel) {
				if(!this.compiled) {
					this.compileDisplayList(var1);
				}

				int var2;
				if(this.rotateAngleX == 0.0F && this.rotateAngleY == 0.0F && this.rotateAngleZ == 0.0F) {
					if(this.rotationPointX == 0.0F && this.rotationPointY == 0.0F && this.rotationPointZ == 0.0F) {
						GL11.glCallList(this.displayList);
						if(this.childModels != null) {
							for(var2 = 0; var2 < this.childModels.size(); ++var2) {
								((ModelRenderer)this.childModels.get(var2)).render(var1);
							}
						}
					} else {
						GL11.glTranslatef(this.rotationPointX * var1, this.rotationPointY * var1, this.rotationPointZ * var1);
						GL11.glCallList(this.displayList);
						if(this.childModels != null) {
							for(var2 = 0; var2 < this.childModels.size(); ++var2) {
								((ModelRenderer)this.childModels.get(var2)).render(var1);
							}
						}

						GL11.glTranslatef(-this.rotationPointX * var1, -this.rotationPointY * var1, -this.rotationPointZ * var1);
					}
				} else {
					GL11.glPushMatrix();
					GL11.glTranslatef(this.rotationPointX * var1, this.rotationPointY * var1, this.rotationPointZ * var1);
					if(this.rotateAngleZ != 0.0F) {
						GL11.glRotatef(this.rotateAngleZ * (180.0F / (float)Math.PI), 0.0F, 0.0F, 1.0F);
					}

					if(this.rotateAngleY != 0.0F) {
						GL11.glRotatef(this.rotateAngleY * (180.0F / (float)Math.PI), 0.0F, 1.0F, 0.0F);
					}

					if(this.rotateAngleX != 0.0F) {
						GL11.glRotatef(this.rotateAngleX * (180.0F / (float)Math.PI), 1.0F, 0.0F, 0.0F);
					}

					GL11.glCallList(this.displayList);
					if(this.childModels != null) {
						for(var2 = 0; var2 < this.childModels.size(); ++var2) {
							((ModelRenderer)this.childModels.get(var2)).render(var1);
						}
					}

					GL11.glPopMatrix();
				}

			}
		}
	}

	public void renderWithRotation(float var1) {
		if(!this.isHidden) {
			if(this.showModel) {
				if(!this.compiled) {
					this.compileDisplayList(var1);
				}

				GL11.glPushMatrix();
				GL11.glTranslatef(this.rotationPointX * var1, this.rotationPointY * var1, this.rotationPointZ * var1);
				if(this.rotateAngleY != 0.0F) {
					GL11.glRotatef(this.rotateAngleY * (180.0F / (float)Math.PI), 0.0F, 1.0F, 0.0F);
				}

				if(this.rotateAngleX != 0.0F) {
					GL11.glRotatef(this.rotateAngleX * (180.0F / (float)Math.PI), 1.0F, 0.0F, 0.0F);
				}

				if(this.rotateAngleZ != 0.0F) {
					GL11.glRotatef(this.rotateAngleZ * (180.0F / (float)Math.PI), 0.0F, 0.0F, 1.0F);
				}

				GL11.glCallList(this.displayList);
				GL11.glPopMatrix();
			}
		}
	}

	public void postRender(float var1) {
		if(!this.isHidden) {
			if(this.showModel) {
				if(!this.compiled) {
					this.compileDisplayList(var1);
				}

				if(this.rotateAngleX == 0.0F && this.rotateAngleY == 0.0F && this.rotateAngleZ == 0.0F) {
					if(this.rotationPointX != 0.0F || this.rotationPointY != 0.0F || this.rotationPointZ != 0.0F) {
						GL11.glTranslatef(this.rotationPointX * var1, this.rotationPointY * var1, this.rotationPointZ * var1);
					}
				} else {
					GL11.glTranslatef(this.rotationPointX * var1, this.rotationPointY * var1, this.rotationPointZ * var1);
					if(this.rotateAngleZ != 0.0F) {
						GL11.glRotatef(this.rotateAngleZ * (180.0F / (float)Math.PI), 0.0F, 0.0F, 1.0F);
					}

					if(this.rotateAngleY != 0.0F) {
						GL11.glRotatef(this.rotateAngleY * (180.0F / (float)Math.PI), 0.0F, 1.0F, 0.0F);
					}

					if(this.rotateAngleX != 0.0F) {
						GL11.glRotatef(this.rotateAngleX * (180.0F / (float)Math.PI), 1.0F, 0.0F, 0.0F);
					}
				}

			}
		}
	}

	private void compileDisplayList(float var1) {
		this.displayList = GLAllocation.generateDisplayLists(1);
		GL11.glNewList(this.displayList, GL11.GL_COMPILE);
		Tessellator var2 = Tessellator.instance;

		for(int var3 = 0; var3 < this.cubeList.size(); ++var3) {
			((ModelBox)this.cubeList.get(var3)).render(var2, var1);
		}

		GL11.glEndList();
		this.compiled = true;
	}

	public ModelRenderer setTextureSize(int var1, int var2) {
		this.textureWidth = (float)var1;
		this.textureHeight = (float)var2;
		return this;
	}
}
