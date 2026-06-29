package net.minecraft.src;

import java.util.Collection;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Random;

public final class SpawnerAnimals {
	private static HashMap eligibleChunksForSpawning = new HashMap();
	protected static final Class[] nightSpawnEntities = new Class[]{EntitySpider.class, EntityZombie.class, EntitySkeleton.class};

	protected static ChunkPosition getRandomSpawningPointInChunk(World var0, int var1, int var2) {
		Chunk var3 = var0.getChunkFromChunkCoords(var1, var2);
		int var4 = var1 * 16 + var0.rand.nextInt(16);
		int var5 = var0.rand.nextInt(var3 == null ? 128 : Math.max(128, var3.getTopFilledSegment()));
		int var6 = var2 * 16 + var0.rand.nextInt(16);
		return new ChunkPosition(var4, var5, var6);
	}

	public static final int performSpawning(World var0, boolean var1, boolean var2) {
		if(!var1 && !var2) {
			return 0;
		} else {
			eligibleChunksForSpawning.clear();

			int var3;
			int var6;
			for(var3 = 0; var3 < var0.playerEntities.size(); ++var3) {
				EntityPlayer var4 = (EntityPlayer)var0.playerEntities.get(var3);
				int var5 = MathHelper.floor_double(var4.posX / 16.0D);
				var6 = MathHelper.floor_double(var4.posZ / 16.0D);
				byte var7 = 8;

				for(int var8 = -var7; var8 <= var7; ++var8) {
					for(int var9 = -var7; var9 <= var7; ++var9) {
						boolean var10 = var8 == -var7 || var8 == var7 || var9 == -var7 || var9 == var7;
						ChunkCoordIntPair var11 = new ChunkCoordIntPair(var8 + var5, var9 + var6);
						if(!var10) {
							eligibleChunksForSpawning.put(var11, Boolean.valueOf(false));
						} else if(!eligibleChunksForSpawning.containsKey(var11)) {
							eligibleChunksForSpawning.put(var11, Boolean.valueOf(true));
						}
					}
				}
			}

			var3 = 0;
			ChunkCoordinates var31 = var0.getSpawnPoint();
			EnumCreatureType[] var32 = EnumCreatureType.values();
			var6 = var32.length;

			label126:
			for(int var33 = 0; var33 < var6; ++var33) {
				EnumCreatureType var34 = var32[var33];
				if((!var34.getPeacefulCreature() || var2) && (var34.getPeacefulCreature() || var1) && var0.countEntities(var34.getCreatureClass()) <= var34.getMaxNumberOfCreature() * eligibleChunksForSpawning.size() / 256) {
					Iterator var35 = eligibleChunksForSpawning.keySet().iterator();

					label123:
					while(true) {
						int var12;
						int var13;
						int var14;
						do {
							do {
								ChunkCoordIntPair var36;
								do {
									if(!var35.hasNext()) {
										continue label126;
									}

									var36 = (ChunkCoordIntPair)var35.next();
								} while(((Boolean)eligibleChunksForSpawning.get(var36)).booleanValue());

								ChunkPosition var37 = getRandomSpawningPointInChunk(var0, var36.chunkXPos, var36.chunkZPos);
								var12 = var37.x;
								var13 = var37.y;
								var14 = var37.z;
							} while(var0.isBlockNormalCube(var12, var13, var14));
						} while(var0.getBlockMaterial(var12, var13, var14) != var34.getCreatureMaterial());

						int var15 = 0;

						for(int var16 = 0; var16 < 3; ++var16) {
							int var17 = var12;
							int var18 = var13;
							int var19 = var14;
							byte var20 = 6;
							SpawnListEntry var21 = null;

							for(int var22 = 0; var22 < 4; ++var22) {
								var17 += var0.rand.nextInt(var20) - var0.rand.nextInt(var20);
								var18 += var0.rand.nextInt(1) - var0.rand.nextInt(1);
								var19 += var0.rand.nextInt(var20) - var0.rand.nextInt(var20);
								if(canCreatureTypeSpawnAtLocation(var34, var0, var17, var18, var19)) {
									float var23 = (float)var17 + 0.5F;
									float var24 = (float)var18;
									float var25 = (float)var19 + 0.5F;
									if(var0.getClosestPlayer((double)var23, (double)var24, (double)var25, 24.0D) == null) {
										float var26 = var23 - (float)var31.posX;
										float var27 = var24 - (float)var31.posY;
										float var28 = var25 - (float)var31.posZ;
										float var29 = var26 * var26 + var27 * var27 + var28 * var28;
										if(var29 >= 576.0F) {
											if(var21 == null) {
												var21 = var0.getRandomMob(var34, var17, var18, var19);
												if(var21 == null) {
													break;
												}
											}

											EntityLiving var38;
											try {
												var38 = (EntityLiving)var21.entityClass.getConstructor(new Class[]{World.class}).newInstance(new Object[]{var0});
											} catch (Exception var30) {
												var30.printStackTrace();
												return var3;
											}

											var38.setLocationAndAngles((double)var23, (double)var24, (double)var25, var0.rand.nextFloat() * 360.0F, 0.0F);
											if(var38.getCanSpawnHere()) {
												++var15;
												var0.spawnEntityInWorld(var38);
												creatureSpecificInit(var38, var0, var23, var24, var25);
												if(var15 >= var38.getMaxSpawnedInChunk()) {
													continue label123;
												}
											}

											var3 += var15;
										}
									}
								}
							}
						}
					}
				}
			}

			return var3;
		}
	}

	public static boolean canCreatureTypeSpawnAtLocation(EnumCreatureType var0, World var1, int var2, int var3, int var4) {
		if(var0.getCreatureMaterial() == Material.water) {
			return var1.getBlockMaterial(var2, var3, var4).isLiquid() && !var1.isBlockNormalCube(var2, var3 + 1, var4);
		} else {
			int var5 = var1.getBlockId(var2, var3 - 1, var4);
			return Block.isNormalCube(var5) && var5 != Block.bedrock.blockID && !var1.isBlockNormalCube(var2, var3, var4) && !var1.getBlockMaterial(var2, var3, var4).isLiquid() && !var1.isBlockNormalCube(var2, var3 + 1, var4);
		}
	}

	private static void creatureSpecificInit(EntityLiving var0, World var1, float var2, float var3, float var4) {
		if(var0 instanceof EntitySpider && var1.rand.nextInt(100) == 0) {
			EntitySkeleton var7 = new EntitySkeleton(var1);
			var7.setLocationAndAngles((double)var2, (double)var3, (double)var4, var0.rotationYaw, 0.0F);
			var1.spawnEntityInWorld(var7);
			var7.mountEntity(var0);
		} else if(var0 instanceof EntitySheep) {
			((EntitySheep)var0).setFleeceColor(EntitySheep.getRandomFleeceColor(var1.rand));
		} else if(var0 instanceof EntityOcelot && var1.rand.nextInt(7) == 0) {
			for(int var5 = 0; var5 < 2; ++var5) {
				EntityOcelot var6 = new EntityOcelot(var1);
				var6.setLocationAndAngles((double)var2, (double)var3, (double)var4, var0.rotationYaw, 0.0F);
				var6.setGrowingAge(-24000);
				var1.spawnEntityInWorld(var6);
			}
		}

	}

	public static void performWorldGenSpawning(World var0, BiomeGenBase var1, int var2, int var3, int var4, int var5, Random var6) {
		List var7 = var1.getSpawnableList(EnumCreatureType.creature);
		if(!var7.isEmpty()) {
			while(var6.nextFloat() < var1.getSpawningChance()) {
				SpawnListEntry var8 = (SpawnListEntry)WeightedRandom.getRandomItem(var0.rand, (Collection)var7);
				int var9 = var8.minGroupCount + var6.nextInt(1 + var8.maxGroupCount - var8.minGroupCount);
				int var10 = var2 + var6.nextInt(var4);
				int var11 = var3 + var6.nextInt(var5);
				int var12 = var10;
				int var13 = var11;

				for(int var14 = 0; var14 < var9; ++var14) {
					boolean var15 = false;

					for(int var16 = 0; !var15 && var16 < 4; ++var16) {
						int var17 = var0.getTopSolidOrLiquidBlock(var10, var11);
						if(canCreatureTypeSpawnAtLocation(EnumCreatureType.creature, var0, var10, var17, var11)) {
							float var18 = (float)var10 + 0.5F;
							float var19 = (float)var17;
							float var20 = (float)var11 + 0.5F;

							EntityLiving var21;
							try {
								var21 = (EntityLiving)var8.entityClass.getConstructor(new Class[]{World.class}).newInstance(new Object[]{var0});
							} catch (Exception var23) {
								var23.printStackTrace();
								continue;
							}

							var21.setLocationAndAngles((double)var18, (double)var19, (double)var20, var6.nextFloat() * 360.0F, 0.0F);
							var0.spawnEntityInWorld(var21);
							creatureSpecificInit(var21, var0, var18, var19, var20);
							var15 = true;
						}

						var10 += var6.nextInt(5) - var6.nextInt(5);

						for(var11 += var6.nextInt(5) - var6.nextInt(5); var10 < var2 || var10 >= var2 + var4 || var11 < var3 || var11 >= var3 + var4; var11 = var13 + var6.nextInt(5) - var6.nextInt(5)) {
							var10 = var12 + var6.nextInt(5) - var6.nextInt(5);
						}
					}
				}
			}

		}
	}
}
