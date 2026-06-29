package net.minecraft.src;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

public class StatList {
	protected static Map oneShotStats = new HashMap();
	public static List allStats = new ArrayList();
	public static List generalStats = new ArrayList();
	public static List itemStats = new ArrayList();
	public static List objectMineStats = new ArrayList();
	public static StatBase startGameStat = (new StatBasic(1000, "stat.startGame")).initIndependentStat().registerStat();
	public static StatBase createWorldStat = (new StatBasic(1001, "stat.createWorld")).initIndependentStat().registerStat();
	public static StatBase loadWorldStat = (new StatBasic(1002, "stat.loadWorld")).initIndependentStat().registerStat();
	public static StatBase joinMultiplayerStat = (new StatBasic(1003, "stat.joinMultiplayer")).initIndependentStat().registerStat();
	public static StatBase leaveGameStat = (new StatBasic(1004, "stat.leaveGame")).initIndependentStat().registerStat();
	public static StatBase minutesPlayedStat = (new StatBasic(1100, "stat.playOneMinute", StatBase.timeStatType)).initIndependentStat().registerStat();
	public static StatBase distanceWalkedStat = (new StatBasic(2000, "stat.walkOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceSwumStat = (new StatBasic(2001, "stat.swimOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceFallenStat = (new StatBasic(2002, "stat.fallOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceClimbedStat = (new StatBasic(2003, "stat.climbOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceFlownStat = (new StatBasic(2004, "stat.flyOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceDoveStat = (new StatBasic(2005, "stat.diveOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceByMinecartStat = (new StatBasic(2006, "stat.minecartOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceByBoatStat = (new StatBasic(2007, "stat.boatOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase distanceByPigStat = (new StatBasic(2008, "stat.pigOneCm", StatBase.distanceStatType)).initIndependentStat().registerStat();
	public static StatBase jumpStat = (new StatBasic(2010, "stat.jump")).initIndependentStat().registerStat();
	public static StatBase dropStat = (new StatBasic(2011, "stat.drop")).initIndependentStat().registerStat();
	public static StatBase damageDealtStat = (new StatBasic(2020, "stat.damageDealt")).registerStat();
	public static StatBase damageTakenStat = (new StatBasic(2021, "stat.damageTaken")).registerStat();
	public static StatBase deathsStat = (new StatBasic(2022, "stat.deaths")).registerStat();
	public static StatBase mobKillsStat = (new StatBasic(2023, "stat.mobKills")).registerStat();
	public static StatBase playerKillsStat = (new StatBasic(2024, "stat.playerKills")).registerStat();
	public static StatBase fishCaughtStat = (new StatBasic(2025, "stat.fishCaught")).registerStat();
	public static StatBase[] mineBlockStatArray = initMinableStats("stat.mineBlock", 16777216);
	public static StatBase[] objectCraftStats;
	public static StatBase[] objectUseStats;
	public static StatBase[] objectBreakStats;
	private static boolean blockStatsInitialized;
	private static boolean itemStatsInitialized;

	public static void func_27360_a() {
	}

	public static void initBreakableStats() {
		objectUseStats = initUsableStats(objectUseStats, "stat.useItem", 16908288, 0, 256);
		objectBreakStats = initBreakStats(objectBreakStats, "stat.breakItem", 16973824, 0, 256);
		blockStatsInitialized = true;
		initCraftableStats();
	}

	public static void initStats() {
		objectUseStats = initUsableStats(objectUseStats, "stat.useItem", 16908288, 256, 32000);
		objectBreakStats = initBreakStats(objectBreakStats, "stat.breakItem", 16973824, 256, 32000);
		itemStatsInitialized = true;
		initCraftableStats();
	}

	public static void initCraftableStats() {
		if(blockStatsInitialized && itemStatsInitialized) {
			HashSet var0 = new HashSet();
			Iterator var1 = CraftingManager.getInstance().getRecipeList().iterator();

			while(var1.hasNext()) {
				IRecipe var2 = (IRecipe)var1.next();
				var0.add(Integer.valueOf(var2.getRecipeOutput().itemID));
			}

			var1 = FurnaceRecipes.smelting().getSmeltingList().values().iterator();

			while(var1.hasNext()) {
				ItemStack var4 = (ItemStack)var1.next();
				var0.add(Integer.valueOf(var4.itemID));
			}

			objectCraftStats = new StatBase[32000];
			var1 = var0.iterator();

			while(var1.hasNext()) {
				Integer var5 = (Integer)var1.next();
				if(Item.itemsList[var5.intValue()] != null) {
					String var3 = StatCollector.translateToLocalFormatted("stat.craftItem", new Object[]{Item.itemsList[var5.intValue()].getStatName()});
					objectCraftStats[var5.intValue()] = (new StatCrafting(16842752 + var5.intValue(), var3, var5.intValue())).registerStat();
				}
			}

			replaceAllSimilarBlocks(objectCraftStats);
		}
	}

	private static StatBase[] initMinableStats(String var0, int var1) {
		StatBase[] var2 = new StatBase[256];

		for(int var3 = 0; var3 < 256; ++var3) {
			if(Block.blocksList[var3] != null && Block.blocksList[var3].getEnableStats()) {
				String var4 = StatCollector.translateToLocalFormatted(var0, new Object[]{Block.blocksList[var3].translateBlockName()});
				var2[var3] = (new StatCrafting(var1 + var3, var4, var3)).registerStat();
				objectMineStats.add((StatCrafting)var2[var3]);
			}
		}

		replaceAllSimilarBlocks(var2);
		return var2;
	}

	private static StatBase[] initUsableStats(StatBase[] var0, String var1, int var2, int var3, int var4) {
		if(var0 == null) {
			var0 = new StatBase[32000];
		}

		for(int var5 = var3; var5 < var4; ++var5) {
			if(Item.itemsList[var5] != null) {
				String var6 = StatCollector.translateToLocalFormatted(var1, new Object[]{Item.itemsList[var5].getStatName()});
				var0[var5] = (new StatCrafting(var2 + var5, var6, var5)).registerStat();
				if(var5 >= 256) {
					itemStats.add((StatCrafting)var0[var5]);
				}
			}
		}

		replaceAllSimilarBlocks(var0);
		return var0;
	}

	private static StatBase[] initBreakStats(StatBase[] var0, String var1, int var2, int var3, int var4) {
		if(var0 == null) {
			var0 = new StatBase[32000];
		}

		for(int var5 = var3; var5 < var4; ++var5) {
			if(Item.itemsList[var5] != null && Item.itemsList[var5].isDamageable()) {
				String var6 = StatCollector.translateToLocalFormatted(var1, new Object[]{Item.itemsList[var5].getStatName()});
				var0[var5] = (new StatCrafting(var2 + var5, var6, var5)).registerStat();
			}
		}

		replaceAllSimilarBlocks(var0);
		return var0;
	}

	private static void replaceAllSimilarBlocks(StatBase[] var0) {
		replaceSimilarBlocks(var0, Block.waterStill.blockID, Block.waterMoving.blockID);
		replaceSimilarBlocks(var0, Block.lavaStill.blockID, Block.lavaStill.blockID);
		replaceSimilarBlocks(var0, Block.pumpkinLantern.blockID, Block.pumpkin.blockID);
		replaceSimilarBlocks(var0, Block.stoneOvenActive.blockID, Block.stoneOvenIdle.blockID);
		replaceSimilarBlocks(var0, Block.oreRedstoneGlowing.blockID, Block.oreRedstone.blockID);
		replaceSimilarBlocks(var0, Block.redstoneRepeaterActive.blockID, Block.redstoneRepeaterIdle.blockID);
		replaceSimilarBlocks(var0, Block.torchRedstoneActive.blockID, Block.torchRedstoneIdle.blockID);
		replaceSimilarBlocks(var0, Block.mushroomRed.blockID, Block.mushroomBrown.blockID);
		replaceSimilarBlocks(var0, Block.stairDouble.blockID, Block.stairSingle.blockID);
		replaceSimilarBlocks(var0, Block.grass.blockID, Block.dirt.blockID);
		replaceSimilarBlocks(var0, Block.tilledField.blockID, Block.dirt.blockID);
	}

	private static void replaceSimilarBlocks(StatBase[] var0, int var1, int var2) {
		if(var0[var1] != null && var0[var2] == null) {
			var0[var2] = var0[var1];
		} else {
			allStats.remove(var0[var1]);
			objectMineStats.remove(var0[var1]);
			generalStats.remove(var0[var1]);
			var0[var1] = var0[var2];
		}
	}

	public static StatBase getOneShotStat(int var0) {
		return (StatBase)oneShotStats.get(Integer.valueOf(var0));
	}

	static {
		AchievementList.init();
		blockStatsInitialized = false;
		itemStatsInitialized = false;
	}
}
