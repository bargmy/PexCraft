package net.minecraft.src;

import java.util.HashMap;
import java.util.Iterator;
import java.util.List;

public class ItemPotion extends Item {
	private HashMap effectCache = new HashMap();

	public ItemPotion(int var1) {
		super(var1);
		this.setMaxStackSize(1);
		this.setHasSubtypes(true);
		this.setMaxDamage(0);
	}

	public List getEffects(ItemStack var1) {
		return this.getEffects(var1.getItemDamage());
	}

	public List getEffects(int var1) {
		List var2 = (List)this.effectCache.get(Integer.valueOf(var1));
		if(var2 == null) {
			var2 = PotionHelper.getPotionEffects(var1, false);
			this.effectCache.put(Integer.valueOf(var1), var2);
		}

		return var2;
	}

	public ItemStack onFoodEaten(ItemStack var1, World var2, EntityPlayer var3) {
		--var1.stackSize;
		if(!var2.isRemote) {
			List var4 = this.getEffects(var1);
			if(var4 != null) {
				Iterator var5 = var4.iterator();

				while(var5.hasNext()) {
					PotionEffect var6 = (PotionEffect)var5.next();
					var3.addPotionEffect(new PotionEffect(var6));
				}
			}
		}

		if(var1.stackSize <= 0) {
			return new ItemStack(Item.glassBottle);
		} else {
			var3.inventory.addItemStackToInventory(new ItemStack(Item.glassBottle));
			return var1;
		}
	}

	public int getMaxItemUseDuration(ItemStack var1) {
		return 32;
	}

	public EnumAction getItemUseAction(ItemStack var1) {
		return EnumAction.drink;
	}

	public ItemStack onItemRightClick(ItemStack var1, World var2, EntityPlayer var3) {
		if(isSplash(var1.getItemDamage())) {
			--var1.stackSize;
			var2.playSoundAtEntity(var3, "random.bow", 0.5F, 0.4F / (itemRand.nextFloat() * 0.4F + 0.8F));
			if(!var2.isRemote) {
				var2.spawnEntityInWorld(new EntityPotion(var2, var3, var1.getItemDamage()));
			}

			return var1;
		} else {
			var3.setItemInUse(var1, this.getMaxItemUseDuration(var1));
			return var1;
		}
	}

	public boolean onItemUse(ItemStack var1, EntityPlayer var2, World var3, int var4, int var5, int var6, int var7) {
		return false;
	}

	public int getIconFromDamage(int var1) {
		return isSplash(var1) ? 154 : 140;
	}

	public int func_46057_a(int var1, int var2) {
		return var2 == 0 ? 141 : super.func_46057_a(var1, var2);
	}

	public static boolean isSplash(int var0) {
		return (var0 & 16384) != 0;
	}

	public int getColorFromDamage(int var1, int var2) {
		return var2 > 0 ? 16777215 : PotionHelper.func_40358_a(var1, false);
	}

	public boolean func_46058_c() {
		return true;
	}

	public boolean isEffectInstant(int var1) {
		List var2 = this.getEffects(var1);
		if(var2 != null && !var2.isEmpty()) {
			Iterator var3 = var2.iterator();

			PotionEffect var4;
			do {
				if(!var3.hasNext()) {
					return false;
				}

				var4 = (PotionEffect)var3.next();
			} while(!Potion.potionTypes[var4.getPotionID()].isInstant());

			return true;
		} else {
			return false;
		}
	}

	public String getItemDisplayName(ItemStack var1) {
		if(var1.getItemDamage() == 0) {
			return StatCollector.translateToLocal("item.emptyPotion.name").trim();
		} else {
			String var2 = "";
			if(isSplash(var1.getItemDamage())) {
				var2 = StatCollector.translateToLocal("potion.prefix.grenade").trim() + " ";
			}

			List var3 = Item.potion.getEffects(var1);
			String var4;
			if(var3 != null && !var3.isEmpty()) {
				var4 = ((PotionEffect)var3.get(0)).getEffectName();
				var4 = var4 + ".postfix";
				return var2 + StatCollector.translateToLocal(var4).trim();
			} else {
				var4 = PotionHelper.func_40359_b(var1.getItemDamage());
				return StatCollector.translateToLocal(var4).trim() + " " + super.getItemDisplayName(var1);
			}
		}
	}

	public void addInformation(ItemStack var1, List var2) {
		if(var1.getItemDamage() != 0) {
			List var3 = Item.potion.getEffects(var1);
			if(var3 != null && !var3.isEmpty()) {
				Iterator var7 = var3.iterator();

				while(var7.hasNext()) {
					PotionEffect var5 = (PotionEffect)var7.next();
					String var6 = StatCollector.translateToLocal(var5.getEffectName()).trim();
					if(var5.getAmplifier() > 0) {
						var6 = var6 + " " + StatCollector.translateToLocal("potion.potency." + var5.getAmplifier()).trim();
					}

					if(var5.getDuration() > 20) {
						var6 = var6 + " (" + Potion.getDurationString(var5) + ")";
					}

					if(Potion.potionTypes[var5.getPotionID()].isBadEffect()) {
						var2.add("\u00a7c" + var6);
					} else {
						var2.add("\u00a77" + var6);
					}
				}
			} else {
				String var4 = StatCollector.translateToLocal("potion.empty").trim();
				var2.add("\u00a77" + var4);
			}

		}
	}

	public boolean hasEffect(ItemStack var1) {
		List var2 = this.getEffects(var1);
		return var2 != null && !var2.isEmpty();
	}
}
