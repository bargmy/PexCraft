package net.minecraft.src;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Random;

public class EnchantmentHelper {
	private static final Random enchantmentRand = new Random();
	private static final EnchantmentModifierDamage enchantmentModifierDamage = new EnchantmentModifierDamage((Empty3)null);
	private static final EnchantmentModifierLiving enchantmentModifierLiving = new EnchantmentModifierLiving((Empty3)null);

	public static int getEnchantmentLevel(int var0, ItemStack var1) {
		if(var1 == null) {
			return 0;
		} else {
			NBTTagList var2 = var1.getEnchantmentTagList();
			if(var2 == null) {
				return 0;
			} else {
				for(int var3 = 0; var3 < var2.tagCount(); ++var3) {
					short var4 = ((NBTTagCompound)var2.tagAt(var3)).getShort("id");
					short var5 = ((NBTTagCompound)var2.tagAt(var3)).getShort("lvl");
					if(var4 == var0) {
						return var5;
					}
				}

				return 0;
			}
		}
	}

	private static int getMaxEnchantmentLevel(int var0, ItemStack[] var1) {
		int var2 = 0;
		ItemStack[] var3 = var1;
		int var4 = var1.length;

		for(int var5 = 0; var5 < var4; ++var5) {
			ItemStack var6 = var3[var5];
			int var7 = getEnchantmentLevel(var0, var6);
			if(var7 > var2) {
				var2 = var7;
			}
		}

		return var2;
	}

	private static void applyEnchantmentModifier(IEnchantmentModifier var0, ItemStack var1) {
		if(var1 != null) {
			NBTTagList var2 = var1.getEnchantmentTagList();
			if(var2 != null) {
				for(int var3 = 0; var3 < var2.tagCount(); ++var3) {
					short var4 = ((NBTTagCompound)var2.tagAt(var3)).getShort("id");
					short var5 = ((NBTTagCompound)var2.tagAt(var3)).getShort("lvl");
					if(Enchantment.enchantmentsList[var4] != null) {
						var0.calculateModifier(Enchantment.enchantmentsList[var4], var5);
					}
				}

			}
		}
	}

	private static void applyEnchantmentModifierArray(IEnchantmentModifier var0, ItemStack[] var1) {
		ItemStack[] var2 = var1;
		int var3 = var1.length;

		for(int var4 = 0; var4 < var3; ++var4) {
			ItemStack var5 = var2[var4];
			applyEnchantmentModifier(var0, var5);
		}

	}

	public static int getEnchantmentModifierDamage(InventoryPlayer var0, DamageSource var1) {
		enchantmentModifierDamage.damageModifier = 0;
		enchantmentModifierDamage.damageSource = var1;
		applyEnchantmentModifierArray(enchantmentModifierDamage, var0.armorInventory);
		if(enchantmentModifierDamage.damageModifier > 25) {
			enchantmentModifierDamage.damageModifier = 25;
		}

		return (enchantmentModifierDamage.damageModifier + 1 >> 1) + enchantmentRand.nextInt((enchantmentModifierDamage.damageModifier >> 1) + 1);
	}

	public static int getEnchantmentModifierLiving(InventoryPlayer var0, EntityLiving var1) {
		enchantmentModifierLiving.livingModifier = 0;
		enchantmentModifierLiving.entityLiving = var1;
		applyEnchantmentModifier(enchantmentModifierLiving, var0.getCurrentItem());
		return enchantmentModifierLiving.livingModifier > 0 ? 1 + enchantmentRand.nextInt(enchantmentModifierLiving.livingModifier) : 0;
	}

	public static int getKnockbackModifier(InventoryPlayer var0, EntityLiving var1) {
		return getEnchantmentLevel(Enchantment.knockback.effectId, var0.getCurrentItem());
	}

	public static int getFireAspectModifier(InventoryPlayer var0, EntityLiving var1) {
		return getEnchantmentLevel(Enchantment.fireAspect.effectId, var0.getCurrentItem());
	}

	public static int getRespiration(InventoryPlayer var0) {
		return getMaxEnchantmentLevel(Enchantment.respiration.effectId, var0.armorInventory);
	}

	public static int getEfficiencyModifier(InventoryPlayer var0) {
		return getEnchantmentLevel(Enchantment.efficiency.effectId, var0.getCurrentItem());
	}

	public static int getUnbreakingModifier(InventoryPlayer var0) {
		return getEnchantmentLevel(Enchantment.unbreaking.effectId, var0.getCurrentItem());
	}

	public static boolean getSilkTouchModifier(InventoryPlayer var0) {
		return getEnchantmentLevel(Enchantment.silkTouch.effectId, var0.getCurrentItem()) > 0;
	}

	public static int getFortuneModifier(InventoryPlayer var0) {
		return getEnchantmentLevel(Enchantment.fortune.effectId, var0.getCurrentItem());
	}

	public static int getLootingModifier(InventoryPlayer var0) {
		return getEnchantmentLevel(Enchantment.looting.effectId, var0.getCurrentItem());
	}

	public static boolean getAquaAffinityModifier(InventoryPlayer var0) {
		return getMaxEnchantmentLevel(Enchantment.aquaAffinity.effectId, var0.armorInventory) > 0;
	}

	public static int calcItemStackEnchantability(Random var0, int var1, int var2, ItemStack var3) {
		Item var4 = var3.getItem();
		int var5 = var4.getItemEnchantability();
		if(var5 <= 0) {
			return 0;
		} else {
			if(var2 > 30) {
				var2 = 30;
			}

			var2 = 1 + (var2 >> 1) + var0.nextInt(var2 + 1);
			int var6 = var0.nextInt(5) + var2;
			return var1 == 0 ? (var6 >> 1) + 1 : (var1 == 1 ? var6 * 2 / 3 + 1 : var6);
		}
	}

	public static void func_48441_a(Random var0, ItemStack var1, int var2) {
		List var3 = buildEnchantmentList(var0, var1, var2);
		if(var3 != null) {
			Iterator var4 = var3.iterator();

			while(var4.hasNext()) {
				EnchantmentData var5 = (EnchantmentData)var4.next();
				var1.addEnchantment(var5.enchantmentobj, var5.enchantmentLevel);
			}
		}

	}

	public static List buildEnchantmentList(Random var0, ItemStack var1, int var2) {
		Item var3 = var1.getItem();
		int var4 = var3.getItemEnchantability();
		if(var4 <= 0) {
			return null;
		} else {
			var4 = 1 + var0.nextInt((var4 >> 1) + 1) + var0.nextInt((var4 >> 1) + 1);
			int var5 = var4 + var2;
			float var6 = (var0.nextFloat() + var0.nextFloat() - 1.0F) * 0.25F;
			int var7 = (int)((float)var5 * (1.0F + var6) + 0.5F);
			ArrayList var8 = null;
			Map var9 = mapEnchantmentData(var7, var1);
			if(var9 != null && !var9.isEmpty()) {
				EnchantmentData var10 = (EnchantmentData)WeightedRandom.getRandomItem(var0, var9.values());
				if(var10 != null) {
					var8 = new ArrayList();
					var8.add(var10);

					for(int var11 = var7 >> 1; var0.nextInt(50) <= var11; var11 >>= 1) {
						Iterator var12 = var9.keySet().iterator();

						while(var12.hasNext()) {
							Integer var13 = (Integer)var12.next();
							boolean var14 = true;
							Iterator var15 = var8.iterator();

							while(var15.hasNext()) {
								EnchantmentData var16 = (EnchantmentData)var15.next();
								if(!var16.enchantmentobj.canApplyTogether(Enchantment.enchantmentsList[var13.intValue()])) {
									var14 = false;
									break;
								}
							}

							if(!var14) {
								var12.remove();
							}
						}

						if(!var9.isEmpty()) {
							EnchantmentData var17 = (EnchantmentData)WeightedRandom.getRandomItem(var0, var9.values());
							var8.add(var17);
						}
					}
				}
			}

			return var8;
		}
	}

	public static Map mapEnchantmentData(int var0, ItemStack var1) {
		Item var2 = var1.getItem();
		HashMap var3 = null;
		Enchantment[] var4 = Enchantment.enchantmentsList;
		int var5 = var4.length;

		for(int var6 = 0; var6 < var5; ++var6) {
			Enchantment var7 = var4[var6];
			if(var7 != null && var7.type.canEnchantItem(var2)) {
				for(int var8 = var7.getMinLevel(); var8 <= var7.getMaxLevel(); ++var8) {
					if(var0 >= var7.getMinEnchantability(var8) && var0 <= var7.getMaxEnchantability(var8)) {
						if(var3 == null) {
							var3 = new HashMap();
						}

						var3.put(Integer.valueOf(var7.effectId), new EnchantmentData(var7, var8));
					}
				}
			}
		}

		return var3;
	}
}
