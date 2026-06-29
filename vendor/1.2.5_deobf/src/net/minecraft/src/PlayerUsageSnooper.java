package net.minecraft.src;

import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.Map;

public class PlayerUsageSnooper {
	private Map field_52025_a = new HashMap();
	private final URL field_52024_b;

	public PlayerUsageSnooper(String var1) {
		try {
			this.field_52024_b = new URL("http://snoop.minecraft.net/" + var1);
		} catch (MalformedURLException var3) {
			throw new IllegalArgumentException();
		}
	}

	public void func_52022_a(String var1, Object var2) {
		this.field_52025_a.put(var1, var2);
	}

	public void func_52021_a() {
		PlayerUsageSnooperThread var1 = new PlayerUsageSnooperThread(this, "reporter");
		var1.setDaemon(true);
		var1.start();
	}

	static URL func_52023_a(PlayerUsageSnooper var0) {
		return var0.field_52024_b;
	}

	static Map func_52020_b(PlayerUsageSnooper var0) {
		return var0.field_52025_a;
	}
}
