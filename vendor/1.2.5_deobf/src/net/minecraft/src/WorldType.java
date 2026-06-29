package net.minecraft.src;

public class WorldType {
	public static final WorldType[] worldTypes = new WorldType[16];
	public static final WorldType DEFAULT = (new WorldType(0, "default", 1)).func_48631_f();
	public static final WorldType FLAT = new WorldType(1, "flat");
	public static final WorldType DEFAULT_1_1 = (new WorldType(8, "default_1_1", 0)).setCanBeCreated(false);
	private final String worldType;
	private final int generatorVersion;
	private boolean canBeCreated;
	private boolean field_48638_h;

	private WorldType(int var1, String var2) {
		this(var1, var2, 0);
	}

	private WorldType(int var1, String var2, int var3) {
		this.worldType = var2;
		this.generatorVersion = var3;
		this.canBeCreated = true;
		worldTypes[var1] = this;
	}

	public String func_48628_a() {
		return this.worldType;
	}

	public String getTranslateName() {
		return "generator." + this.worldType;
	}

	public int getGeneratorVersion() {
		return this.generatorVersion;
	}

	public WorldType func_48629_a(int var1) {
		return this == DEFAULT && var1 == 0 ? DEFAULT_1_1 : this;
	}

	private WorldType setCanBeCreated(boolean var1) {
		this.canBeCreated = var1;
		return this;
	}

	public boolean getCanBeCreated() {
		return this.canBeCreated;
	}

	private WorldType func_48631_f() {
		this.field_48638_h = true;
		return this;
	}

	public boolean func_48626_e() {
		return this.field_48638_h;
	}

	public static WorldType parseWorldType(String var0) {
		for(int var1 = 0; var1 < worldTypes.length; ++var1) {
			if(worldTypes[var1] != null && worldTypes[var1].worldType.equalsIgnoreCase(var0)) {
				return worldTypes[var1];
			}
		}

		return null;
	}
}
