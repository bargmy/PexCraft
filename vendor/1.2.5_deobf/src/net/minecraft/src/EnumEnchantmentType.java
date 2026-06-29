package net.minecraft.src;

public enum EnumEnchantmentType {
	all,
	armor,
	armor_feet,
	armor_legs,
	armor_torso,
	armor_head,
	weapon,
	digger,
	bow;

	public boolean canEnchantItem(Item var1) {
		if(this == all) {
			return true;
		} else if(var1 instanceof ItemArmor) {
			if(this == armor) {
				return true;
			} else {
				ItemArmor var2 = (ItemArmor)var1;
				return var2.armorType == 0 ? this == armor_head : (var2.armorType == 2 ? this == armor_legs : (var2.armorType == 1 ? this == armor_torso : (var2.armorType == 3 ? this == armor_feet : false)));
			}
		} else {
			return var1 instanceof ItemSword ? this == weapon : (var1 instanceof ItemTool ? this == digger : (var1 instanceof ItemBow ? this == bow : false));
		}
	}
}
