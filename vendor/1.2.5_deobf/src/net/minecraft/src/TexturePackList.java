package net.minecraft.src;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import net.minecraft.client.Minecraft;

public class TexturePackList {
	private List availableTexturePacks = new ArrayList();
	private TexturePackBase defaultTexturePack = new TexturePackDefault();
	public TexturePackBase selectedTexturePack;
	private Map field_6538_d = new HashMap();
	private Minecraft mc;
	private File texturePackDir;
	private String currentTexturePack;

	public TexturePackList(Minecraft var1, File var2) {
		this.mc = var1;
		this.texturePackDir = new File(var2, "texturepacks");
		if(this.texturePackDir.exists()) {
			if(!this.texturePackDir.isDirectory()) {
				this.texturePackDir.delete();
				this.texturePackDir.mkdirs();
			}
		} else {
			this.texturePackDir.mkdirs();
		}

		this.currentTexturePack = var1.gameSettings.skin;
		this.updateAvaliableTexturePacks();
		this.selectedTexturePack.func_6482_a();
	}

	public boolean setTexturePack(TexturePackBase var1) {
		if(var1 == this.selectedTexturePack) {
			return false;
		} else {
			this.selectedTexturePack.closeTexturePackFile();
			this.currentTexturePack = var1.texturePackFileName;
			this.selectedTexturePack = var1;
			this.mc.gameSettings.skin = this.currentTexturePack;
			this.mc.gameSettings.saveOptions();
			this.selectedTexturePack.func_6482_a();
			return true;
		}
	}

	public void updateAvaliableTexturePacks() {
		ArrayList var1 = new ArrayList();
		this.selectedTexturePack = null;
		var1.add(this.defaultTexturePack);
		if(this.texturePackDir.exists() && this.texturePackDir.isDirectory()) {
			File[] var2 = this.texturePackDir.listFiles();
			File[] var3 = var2;
			int var4 = var2.length;

			for(int var5 = 0; var5 < var4; ++var5) {
				File var6 = var3[var5];
				String var7;
				TexturePackBase var13;
				if(var6.isFile() && var6.getName().toLowerCase().endsWith(".zip")) {
					var7 = var6.getName() + ":" + var6.length() + ":" + var6.lastModified();

					try {
						if(!this.field_6538_d.containsKey(var7)) {
							TexturePackCustom var14 = new TexturePackCustom(var6);
							var14.texturePackID = var7;
							this.field_6538_d.put(var7, var14);
							var14.func_6485_a(this.mc);
						}

						var13 = (TexturePackBase)this.field_6538_d.get(var7);
						if(var13.texturePackFileName.equals(this.currentTexturePack)) {
							this.selectedTexturePack = var13;
						}

						var1.add(var13);
					} catch (IOException var10) {
						var10.printStackTrace();
					}
				} else if(var6.isDirectory() && (new File(var6, "pack.txt")).exists()) {
					var7 = var6.getName() + ":folder:" + var6.lastModified();

					try {
						if(!this.field_6538_d.containsKey(var7)) {
							TexturePackFolder var8 = new TexturePackFolder(var6);
							var8.texturePackID = var7;
							this.field_6538_d.put(var7, var8);
							var8.func_6485_a(this.mc);
						}

						var13 = (TexturePackBase)this.field_6538_d.get(var7);
						if(var13.texturePackFileName.equals(this.currentTexturePack)) {
							this.selectedTexturePack = var13;
						}

						var1.add(var13);
					} catch (IOException var9) {
						var9.printStackTrace();
					}
				}
			}
		}

		if(this.selectedTexturePack == null) {
			this.selectedTexturePack = this.defaultTexturePack;
		}

		this.availableTexturePacks.removeAll(var1);
		Iterator var11 = this.availableTexturePacks.iterator();

		while(var11.hasNext()) {
			TexturePackBase var12 = (TexturePackBase)var11.next();
			var12.unbindThumbnailTexture(this.mc);
			this.field_6538_d.remove(var12.texturePackID);
		}

		this.availableTexturePacks = var1;
	}

	public List availableTexturePacks() {
		return new ArrayList(this.availableTexturePacks);
	}
}
