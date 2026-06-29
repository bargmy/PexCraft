package net.minecraft.src;

import java.awt.image.BufferedImage;
import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import javax.imageio.ImageIO;
import net.minecraft.client.Minecraft;
import org.lwjgl.opengl.GL11;

public class TexturePackFolder extends TexturePackBase {
	private int field_48191_e = -1;
	private BufferedImage field_48189_f;
	private File field_48190_g;

	public TexturePackFolder(File var1) {
		this.texturePackFileName = var1.getName();
		this.field_48190_g = var1;
	}

	private String func_48188_b(String var1) {
		if(var1 != null && var1.length() > 34) {
			var1 = var1.substring(0, 34);
		}

		return var1;
	}

	public void func_6485_a(Minecraft var1) throws IOException {
		InputStream var2 = null;

		try {
			try {
				var2 = this.getResourceAsStream("pack.txt");
				BufferedReader var3 = new BufferedReader(new InputStreamReader(var2));
				this.firstDescriptionLine = this.func_48188_b(var3.readLine());
				this.secondDescriptionLine = this.func_48188_b(var3.readLine());
				var3.close();
				var2.close();
			} catch (Exception var15) {
			}

			try {
				var2 = this.getResourceAsStream("pack.png");
				this.field_48189_f = ImageIO.read(var2);
				var2.close();
			} catch (Exception var14) {
			}
		} catch (Exception var16) {
			var16.printStackTrace();
		} finally {
			try {
				var2.close();
			} catch (Exception var13) {
			}

		}

	}

	public void unbindThumbnailTexture(Minecraft var1) {
		if(this.field_48189_f != null) {
			var1.renderEngine.deleteTexture(this.field_48191_e);
		}

		this.closeTexturePackFile();
	}

	public void bindThumbnailTexture(Minecraft var1) {
		if(this.field_48189_f != null && this.field_48191_e < 0) {
			this.field_48191_e = var1.renderEngine.allocateAndSetupTexture(this.field_48189_f);
		}

		if(this.field_48189_f != null) {
			var1.renderEngine.bindTexture(this.field_48191_e);
		} else {
			GL11.glBindTexture(GL11.GL_TEXTURE_2D, var1.renderEngine.getTexture("/gui/unknown_pack.png"));
		}

	}

	public void func_6482_a() {
	}

	public void closeTexturePackFile() {
	}

	public InputStream getResourceAsStream(String var1) {
		try {
			File var2 = new File(this.field_48190_g, var1.substring(1));
			if(var2.exists()) {
				return new BufferedInputStream(new FileInputStream(var2));
			}
		} catch (Exception var3) {
		}

		return TexturePackBase.class.getResourceAsStream(var1);
	}
}
