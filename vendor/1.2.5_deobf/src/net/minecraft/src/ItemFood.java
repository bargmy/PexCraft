package net.minecraft.src;

public class ItemFood extends Item {
	public final int field_35430_a;
	private final int healAmount;
	private final float saturationModifier;
	private final boolean isWolfsFavoriteMeat;
	private boolean alwaysEdible;
	private int potionId;
	private int potionDuration;
	private int potionAmplifier;
	private float potionEffectProbability;

	public ItemFood(int var1, int var2, float var3, boolean var4) {
		super(var1);
		this.field_35430_a = 32;
		this.healAmount = var2;
		this.isWolfsFavoriteMeat = var4;
		this.saturationModifier = var3;
	}

	public ItemFood(int var1, int var2, boolean var3) {
		this(var1, var2, 0.6F, var3);
	}

	public ItemStack onFoodEaten(ItemStack var1, World var2, EntityPlayer var3) {
		--var1.stackSize;
		var3.getFoodStats().addStats(this);
		var2.playSoundAtEntity(var3, "random.burp", 0.5F, var2.rand.nextFloat() * 0.1F + 0.9F);
		if(!var2.isRemote && this.potionId > 0 && var2.rand.nextFloat() < this.potionEffectProbability) {
			var3.addPotionEffect(new PotionEffect(this.potionId, this.potionDuration * 20, this.potionAmplifier));
		}

		return var1;
	}

	public int getMaxItemUseDuration(ItemStack var1) {
		return 32;
	}

	public EnumAction getItemUseAction(ItemStack var1) {
		return EnumAction.eat;
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		if(var3.canEat(this.alwaysEdible)) {
			var3.setItemInUse(var1, this.getMaxItemUseDuration(var1));
		}

		return var1;
	}

	public int getHealAmount() {
		return this.healAmount;
	}

	public float getSaturationModifier() {
		return this.saturationModifier;
	}

	public boolean isWolfsFavoriteMeat() {
		return this.isWolfsFavoriteMeat;
	}

	public ItemFood setPotionEffect(int var1, int var2, int var3, float var4) {
		this.potionId = var1;
		this.potionDuration = var2;
		this.potionAmplifier = var3;
		this.potionEffectProbability = var4;
		return this;
	}

	public ItemFood setAlwaysEdible() {
		this.alwaysEdible = true;
		return this;
	}

	public Item setItemName(String var1) {
		return super.setItemName(var1);
	}
}
