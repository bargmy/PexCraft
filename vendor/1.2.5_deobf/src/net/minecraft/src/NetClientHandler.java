package net.minecraft.src;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.DataInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.InetAddress;
import java.net.Socket;
import java.net.URL;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;
import net.minecraft.client.Minecraft;

public class NetClientHandler extends NetHandler {
	private boolean disconnected = false;
	private NetworkManager netManager;
	public String field_1209_a;
	private Minecraft mc;
	private WorldClient worldClient;
	private boolean field_1210_g = false;
	public MapStorage mapStorage = new MapStorage((ISaveHandler)null);
	private Map playerInfoMap = new HashMap();
	public List playerNames = new ArrayList();
	public int currentServerMaxPlayers = 20;
	Random rand = new Random();

	public NetClientHandler(Minecraft var1, String var2, int var3) throws IOException, UnknownHostException {
		this.mc = var1;
		Socket var4 = new Socket(InetAddress.getByName(var2), var3);
		this.netManager = new NetworkManager(var4, "Client", this);
	}

	public void processReadPackets() {
		if(!this.disconnected) {
			this.netManager.processReadPackets();
		}

		this.netManager.wakeThreads();
	}

	public void handleLogin(Packet1Login var1) {
		this.mc.playerController = new PlayerControllerMP(this.mc, this);
		this.mc.statFileWriter.readStat(StatList.joinMultiplayerStat, 1);
		this.worldClient = new WorldClient(this, new WorldSettings(0L, var1.serverMode, false, false, var1.terrainType), var1.field_48170_e, var1.difficultySetting);
		this.worldClient.isRemote = true;
		this.mc.changeWorld1(this.worldClient);
		this.mc.thePlayer.dimension = var1.field_48170_e;
		this.mc.displayGuiScreen(new GuiDownloadTerrain(this));
		this.mc.thePlayer.entityId = var1.protocolVersion;
		this.currentServerMaxPlayers = var1.maxPlayers;
		((PlayerControllerMP)this.mc.playerController).setCreative(var1.serverMode == 1);
	}

	public void handlePickupSpawn(Packet21PickupSpawn var1) {
		double var2 = (double)var1.xPosition / 32.0D;
		double var4 = (double)var1.yPosition / 32.0D;
		double var6 = (double)var1.zPosition / 32.0D;
		EntityItem var8 = new EntityItem(this.worldClient, var2, var4, var6, new ItemStack(var1.itemID, var1.count, var1.itemDamage));
		var8.motionX = (double)var1.rotation / 128.0D;
		var8.motionY = (double)var1.pitch / 128.0D;
		var8.motionZ = (double)var1.roll / 128.0D;
		var8.serverPosX = var1.xPosition;
		var8.serverPosY = var1.yPosition;
		var8.serverPosZ = var1.zPosition;
		this.worldClient.addEntityToWorld(var1.entityId, var8);
	}

	public void handleVehicleSpawn(Packet23VehicleSpawn var1) {
		double var2 = (double)var1.xPosition / 32.0D;
		double var4 = (double)var1.yPosition / 32.0D;
		double var6 = (double)var1.zPosition / 32.0D;
		Object var8 = null;
		if(var1.type == 10) {
			var8 = new EntityMinecart(this.worldClient, var2, var4, var6, 0);
		} else if(var1.type == 11) {
			var8 = new EntityMinecart(this.worldClient, var2, var4, var6, 1);
		} else if(var1.type == 12) {
			var8 = new EntityMinecart(this.worldClient, var2, var4, var6, 2);
		} else if(var1.type == 90) {
			var8 = new EntityFishHook(this.worldClient, var2, var4, var6);
		} else if(var1.type == 60) {
			var8 = new EntityArrow(this.worldClient, var2, var4, var6);
		} else if(var1.type == 61) {
			var8 = new EntitySnowball(this.worldClient, var2, var4, var6);
		} else if(var1.type == 65) {
			var8 = new EntityEnderPearl(this.worldClient, var2, var4, var6);
		} else if(var1.type == 72) {
			var8 = new EntityEnderEye(this.worldClient, var2, var4, var6);
		} else if(var1.type == 63) {
			var8 = new EntityFireball(this.worldClient, var2, var4, var6, (double)var1.speedX / 8000.0D, (double)var1.speedY / 8000.0D, (double)var1.speedZ / 8000.0D);
			var1.throwerEntityId = 0;
		} else if(var1.type == 64) {
			var8 = new EntitySmallFireball(this.worldClient, var2, var4, var6, (double)var1.speedX / 8000.0D, (double)var1.speedY / 8000.0D, (double)var1.speedZ / 8000.0D);
			var1.throwerEntityId = 0;
		} else if(var1.type == 62) {
			var8 = new EntityEgg(this.worldClient, var2, var4, var6);
		} else if(var1.type == 73) {
			var8 = new EntityPotion(this.worldClient, var2, var4, var6, var1.throwerEntityId);
			var1.throwerEntityId = 0;
		} else if(var1.type == 75) {
			var8 = new EntityExpBottle(this.worldClient, var2, var4, var6);
			var1.throwerEntityId = 0;
		} else if(var1.type == 1) {
			var8 = new EntityBoat(this.worldClient, var2, var4, var6);
		} else if(var1.type == 50) {
			var8 = new EntityTNTPrimed(this.worldClient, var2, var4, var6);
		} else if(var1.type == 51) {
			var8 = new EntityEnderCrystal(this.worldClient, var2, var4, var6);
		} else if(var1.type == 70) {
			var8 = new EntityFallingSand(this.worldClient, var2, var4, var6, Block.sand.blockID);
		} else if(var1.type == 71) {
			var8 = new EntityFallingSand(this.worldClient, var2, var4, var6, Block.gravel.blockID);
		} else if(var1.type == 74) {
			var8 = new EntityFallingSand(this.worldClient, var2, var4, var6, Block.dragonEgg.blockID);
		}

		if(var8 != null) {
			((Entity)var8).serverPosX = var1.xPosition;
			((Entity)var8).serverPosY = var1.yPosition;
			((Entity)var8).serverPosZ = var1.zPosition;
			((Entity)var8).rotationYaw = 0.0F;
			((Entity)var8).rotationPitch = 0.0F;
			Entity[] var9 = ((Entity)var8).getParts();
			if(var9 != null) {
				int var10 = var1.entityId - ((Entity)var8).entityId;

				for(int var11 = 0; var11 < var9.length; ++var11) {
					var9[var11].entityId += var10;
				}
			}

			((Entity)var8).entityId = var1.entityId;
			this.worldClient.addEntityToWorld(var1.entityId, (Entity)var8);
			if(var1.throwerEntityId > 0) {
				if(var1.type == 60) {
					Entity var12 = this.getEntityByID(var1.throwerEntityId);
					if(var12 instanceof EntityLiving) {
						((EntityArrow)var8).shootingEntity = (EntityLiving)var12;
					}
				}

				((Entity)var8).setVelocity((double)var1.speedX / 8000.0D, (double)var1.speedY / 8000.0D, (double)var1.speedZ / 8000.0D);
			}
		}

	}

	public void handleEntityExpOrb(Packet26EntityExpOrb var1) {
		EntityXPOrb var2 = new EntityXPOrb(this.worldClient, (double)var1.posX, (double)var1.posY, (double)var1.posZ, var1.xpValue);
		var2.serverPosX = var1.posX;
		var2.serverPosY = var1.posY;
		var2.serverPosZ = var1.posZ;
		var2.rotationYaw = 0.0F;
		var2.rotationPitch = 0.0F;
		var2.entityId = var1.entityId;
		this.worldClient.addEntityToWorld(var1.entityId, var2);
	}

	public void handleWeather(Packet71Weather var1) {
		double var2 = (double)var1.posX / 32.0D;
		double var4 = (double)var1.posY / 32.0D;
		double var6 = (double)var1.posZ / 32.0D;
		EntityLightningBolt var8 = null;
		if(var1.isLightningBolt == 1) {
			var8 = new EntityLightningBolt(this.worldClient, var2, var4, var6);
		}

		if(var8 != null) {
			var8.serverPosX = var1.posX;
			var8.serverPosY = var1.posY;
			var8.serverPosZ = var1.posZ;
			var8.rotationYaw = 0.0F;
			var8.rotationPitch = 0.0F;
			var8.entityId = var1.entityID;
			this.worldClient.addWeatherEffect(var8);
		}

	}

	public void handleEntityPainting(Packet25EntityPainting var1) {
		EntityPainting var2 = new EntityPainting(this.worldClient, var1.xPosition, var1.yPosition, var1.zPosition, var1.direction, var1.title);
		this.worldClient.addEntityToWorld(var1.entityId, var2);
	}

	public void handleEntityVelocity(Packet28EntityVelocity var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null) {
			var2.setVelocity((double)var1.motionX / 8000.0D, (double)var1.motionY / 8000.0D, (double)var1.motionZ / 8000.0D);
		}
	}

	public void handleEntityMetadata(Packet40EntityMetadata var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null && var1.getMetadata() != null) {
			var2.getDataWatcher().updateWatchedObjectsFromList(var1.getMetadata());
		}

	}

	public void handleNamedEntitySpawn(Packet20NamedEntitySpawn var1) {
		double var2 = (double)var1.xPosition / 32.0D;
		double var4 = (double)var1.yPosition / 32.0D;
		double var6 = (double)var1.zPosition / 32.0D;
		float var8 = (float)(var1.rotation * 360) / 256.0F;
		float var9 = (float)(var1.pitch * 360) / 256.0F;
		EntityOtherPlayerMP var10 = new EntityOtherPlayerMP(this.mc.theWorld, var1.name);
		var10.prevPosX = var10.lastTickPosX = (double)(var10.serverPosX = var1.xPosition);
		var10.prevPosY = var10.lastTickPosY = (double)(var10.serverPosY = var1.yPosition);
		var10.prevPosZ = var10.lastTickPosZ = (double)(var10.serverPosZ = var1.zPosition);
		int var11 = var1.currentItem;
		if(var11 == 0) {
			var10.inventory.mainInventory[var10.inventory.currentItem] = null;
		} else {
			var10.inventory.mainInventory[var10.inventory.currentItem] = new ItemStack(var11, 1, 0);
		}

		var10.setPositionAndRotation(var2, var4, var6, var8, var9);
		this.worldClient.addEntityToWorld(var1.entityId, var10);
	}

	public void handleEntityTeleport(Packet34EntityTeleport var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null) {
			var2.serverPosX = var1.xPosition;
			var2.serverPosY = var1.yPosition;
			var2.serverPosZ = var1.zPosition;
			double var3 = (double)var2.serverPosX / 32.0D;
			double var5 = (double)var2.serverPosY / 32.0D + 1.0D / 64.0D;
			double var7 = (double)var2.serverPosZ / 32.0D;
			float var9 = (float)(var1.yaw * 360) / 256.0F;
			float var10 = (float)(var1.pitch * 360) / 256.0F;
			var2.setPositionAndRotation2(var3, var5, var7, var9, var10, 3);
		}
	}

	public void handleEntity(Packet30Entity var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null) {
			var2.serverPosX += var1.xPosition;
			var2.serverPosY += var1.yPosition;
			var2.serverPosZ += var1.zPosition;
			double var3 = (double)var2.serverPosX / 32.0D;
			double var5 = (double)var2.serverPosY / 32.0D;
			double var7 = (double)var2.serverPosZ / 32.0D;
			float var9 = var1.rotating ? (float)(var1.yaw * 360) / 256.0F : var2.rotationYaw;
			float var10 = var1.rotating ? (float)(var1.pitch * 360) / 256.0F : var2.rotationPitch;
			var2.setPositionAndRotation2(var3, var5, var7, var9, var10, 3);
		}
	}

	public void handleEntityHeadRotation(Packet35EntityHeadRotation var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null) {
			float var3 = (float)(var1.headRotationYaw * 360) / 256.0F;
			var2.func_48079_f(var3);
		}
	}

	public void handleDestroyEntity(Packet29DestroyEntity var1) {
		this.worldClient.removeEntityFromWorld(var1.entityId);
	}

	public void handleFlying(Packet10Flying var1) {
		EntityPlayerSP var2 = this.mc.thePlayer;
		double var3 = var2.posX;
		double var5 = var2.posY;
		double var7 = var2.posZ;
		float var9 = var2.rotationYaw;
		float var10 = var2.rotationPitch;
		if(var1.moving) {
			var3 = var1.xPosition;
			var5 = var1.yPosition;
			var7 = var1.zPosition;
		}

		if(var1.rotating) {
			var9 = var1.yaw;
			var10 = var1.pitch;
		}

		var2.ySize = 0.0F;
		var2.motionX = var2.motionY = var2.motionZ = 0.0D;
		var2.setPositionAndRotation(var3, var5, var7, var9, var10);
		var1.xPosition = var2.posX;
		var1.yPosition = var2.boundingBox.minY;
		var1.zPosition = var2.posZ;
		var1.stance = var2.posY;
		this.netManager.addToSendQueue(var1);
		if(!this.field_1210_g) {
			this.mc.thePlayer.prevPosX = this.mc.thePlayer.posX;
			this.mc.thePlayer.prevPosY = this.mc.thePlayer.posY;
			this.mc.thePlayer.prevPosZ = this.mc.thePlayer.posZ;
			this.field_1210_g = true;
			this.mc.displayGuiScreen((GuiScreen)null);
		}

	}

	public void handlePreChunk(Packet50PreChunk var1) {
		this.worldClient.doPreChunk(var1.xPosition, var1.yPosition, var1.mode);
	}

	public void handleMultiBlockChange(Packet52MultiBlockChange var1) {
		int var2 = var1.xPosition * 16;
		int var3 = var1.zPosition * 16;
		if(var1.metadataArray != null) {
			DataInputStream var4 = new DataInputStream(new ByteArrayInputStream(var1.metadataArray));

			try {
				for(int var5 = 0; var5 < var1.size; ++var5) {
					short var6 = var4.readShort();
					short var7 = var4.readShort();
					int var8 = (var7 & 4095) >> 4;
					int var9 = var7 & 15;
					int var10 = var6 >> 12 & 15;
					int var11 = var6 >> 8 & 15;
					int var12 = var6 & 255;
					this.worldClient.setBlockAndMetadataAndInvalidate(var10 + var2, var12, var11 + var3, var8, var9);
				}
			} catch (IOException var13) {
			}

		}
	}

	public void func_48487_a(Packet51MapChunk var1) {
		this.worldClient.invalidateBlockReceiveRegion(var1.xCh << 4, 0, var1.zCh << 4, (var1.xCh << 4) + 15, 256, (var1.zCh << 4) + 15);
		Chunk var2 = this.worldClient.getChunkFromChunkCoords(var1.xCh, var1.zCh);
		if(var1.includeInitialize && var2 == null) {
			this.worldClient.doPreChunk(var1.xCh, var1.zCh, true);
			var2 = this.worldClient.getChunkFromChunkCoords(var1.xCh, var1.zCh);
		}

		if(var2 != null) {
			var2.func_48494_a(var1.chunkData, var1.yChMin, var1.yChMax, var1.includeInitialize);
			this.worldClient.markBlocksDirty(var1.xCh << 4, 0, var1.zCh << 4, (var1.xCh << 4) + 15, 256, (var1.zCh << 4) + 15);
			if(!var1.includeInitialize || !(this.worldClient.worldProvider instanceof WorldProviderSurface)) {
				var2.resetRelightChecks();
			}
		}

	}

	public void handleBlockChange(Packet53BlockChange var1) {
		this.worldClient.setBlockAndMetadataAndInvalidate(var1.xPosition, var1.yPosition, var1.zPosition, var1.type, var1.metadata);
	}

	public void handleKickDisconnect(Packet255KickDisconnect var1) {
		this.netManager.networkShutdown("disconnect.kicked", new Object[0]);
		this.disconnected = true;
		this.mc.changeWorld1((World)null);
		this.mc.displayGuiScreen(new GuiDisconnected("disconnect.disconnected", "disconnect.genericReason", new Object[]{var1.reason}));
	}

	public void handleErrorMessage(String var1, Object[] var2) {
		if(!this.disconnected) {
			this.disconnected = true;
			this.mc.changeWorld1((World)null);
			this.mc.displayGuiScreen(new GuiDisconnected("disconnect.lost", var1, var2));
		}
	}

	public void quitWithPacket(Packet var1) {
		if(!this.disconnected) {
			this.netManager.addToSendQueue(var1);
			this.netManager.serverShutdown();
		}
	}

	public void addToSendQueue(Packet var1) {
		if(!this.disconnected) {
			this.netManager.addToSendQueue(var1);
		}
	}

	public void handleCollect(Packet22Collect var1) {
		Entity var2 = this.getEntityByID(var1.collectedEntityId);
		Object var3 = (EntityLiving)this.getEntityByID(var1.collectorEntityId);
		if(var3 == null) {
			var3 = this.mc.thePlayer;
		}

		if(var2 != null) {
			if(var2 instanceof EntityXPOrb) {
				this.worldClient.playSoundAtEntity(var2, "random.orb", 0.2F, ((this.rand.nextFloat() - this.rand.nextFloat()) * 0.7F + 1.0F) * 2.0F);
			} else {
				this.worldClient.playSoundAtEntity(var2, "random.pop", 0.2F, ((this.rand.nextFloat() - this.rand.nextFloat()) * 0.7F + 1.0F) * 2.0F);
			}

			this.mc.effectRenderer.addEffect(new EntityPickupFX(this.mc.theWorld, var2, (Entity)var3, -0.5F));
			this.worldClient.removeEntityFromWorld(var1.collectedEntityId);
		}

	}

	public void handleChat(Packet3Chat var1) {
		this.mc.ingameGUI.addChatMessage(var1.message);
	}

	public void handleAnimation(Packet18Animation var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null) {
			EntityPlayer var3;
			if(var1.animate == 1) {
				var3 = (EntityPlayer)var2;
				var3.swingItem();
			} else if(var1.animate == 2) {
				var2.performHurtAnimation();
			} else if(var1.animate == 3) {
				var3 = (EntityPlayer)var2;
				var3.wakeUpPlayer(false, false, false);
			} else if(var1.animate == 4) {
				var3 = (EntityPlayer)var2;
				var3.func_6420_o();
			} else if(var1.animate == 6) {
				this.mc.effectRenderer.addEffect(new EntityCrit2FX(this.mc.theWorld, var2));
			} else if(var1.animate == 7) {
				EntityCrit2FX var4 = new EntityCrit2FX(this.mc.theWorld, var2, "magicCrit");
				this.mc.effectRenderer.addEffect(var4);
			} else if(var1.animate == 5 && var2 instanceof EntityOtherPlayerMP) {
			}

		}
	}

	public void handleSleep(Packet17Sleep var1) {
		Entity var2 = this.getEntityByID(var1.entityID);
		if(var2 != null) {
			if(var1.field_22046_e == 0) {
				EntityPlayer var3 = (EntityPlayer)var2;
				var3.sleepInBedAt(var1.bedX, var1.bedY, var1.bedZ);
			}

		}
	}

	public void handleHandshake(Packet2Handshake var1) {
		boolean var2 = true;
		String var3 = var1.username;
		if(var3 != null && var3.trim().length() != 0) {
			if(!var3.equals("-")) {
				try {
					Long.parseLong(var3, 16);
				} catch (NumberFormatException var8) {
					var2 = false;
				}
			}
		} else {
			var2 = false;
		}

		if(!var2) {
			this.netManager.networkShutdown("disconnect.genericReason", new Object[]{"The server responded with an invalid server key"});
		} else if(var1.username.equals("-")) {
			this.addToSendQueue(new Packet1Login(this.mc.session.username, 29));
		} else {
			try {
				URL var4 = new URL("http://session.minecraft.net/game/joinserver.jsp?user=" + this.mc.session.username + "&sessionId=" + this.mc.session.sessionId + "&serverId=" + var1.username);
				BufferedReader var5 = new BufferedReader(new InputStreamReader(var4.openStream()));
				String var6 = var5.readLine();
				var5.close();
				if(var6.equalsIgnoreCase("ok")) {
					this.addToSendQueue(new Packet1Login(this.mc.session.username, 29));
				} else {
					this.netManager.networkShutdown("disconnect.loginFailedInfo", new Object[]{var6});
				}
			} catch (Exception var7) {
				var7.printStackTrace();
				this.netManager.networkShutdown("disconnect.genericReason", new Object[]{"Internal client error: " + var7.toString()});
			}
		}

	}

	public void disconnect() {
		this.disconnected = true;
		this.netManager.wakeThreads();
		this.netManager.networkShutdown("disconnect.closed", new Object[0]);
	}

	public void handleMobSpawn(Packet24MobSpawn var1) {
		double var2 = (double)var1.xPosition / 32.0D;
		double var4 = (double)var1.yPosition / 32.0D;
		double var6 = (double)var1.zPosition / 32.0D;
		float var8 = (float)(var1.yaw * 360) / 256.0F;
		float var9 = (float)(var1.pitch * 360) / 256.0F;
		EntityLiving var10 = (EntityLiving)EntityList.createEntityByID(var1.type, this.mc.theWorld);
		var10.serverPosX = var1.xPosition;
		var10.serverPosY = var1.yPosition;
		var10.serverPosZ = var1.zPosition;
		var10.rotationYawHead = (float)(var1.field_48169_h * 360) / 256.0F;
		Entity[] var11 = var10.getParts();
		if(var11 != null) {
			int var12 = var1.entityId - var10.entityId;

			for(int var13 = 0; var13 < var11.length; ++var13) {
				var11[var13].entityId += var12;
			}
		}

		var10.entityId = var1.entityId;
		var10.setPositionAndRotation(var2, var4, var6, var8, var9);
		this.worldClient.addEntityToWorld(var1.entityId, var10);
		List var14 = var1.getMetadata();
		if(var14 != null) {
			var10.getDataWatcher().updateWatchedObjectsFromList(var14);
		}

	}

	public void handleUpdateTime(Packet4UpdateTime var1) {
		this.mc.theWorld.setWorldTime(var1.time);
	}

	public void handleSpawnPosition(Packet6SpawnPosition var1) {
		this.mc.thePlayer.setSpawnChunk(new ChunkCoordinates(var1.xPosition, var1.yPosition, var1.zPosition));
		this.mc.theWorld.getWorldInfo().setSpawnPosition(var1.xPosition, var1.yPosition, var1.zPosition);
	}

	public void handleAttachEntity(Packet39AttachEntity var1) {
		Object var2 = this.getEntityByID(var1.entityId);
		Entity var3 = this.getEntityByID(var1.vehicleEntityId);
		if(var1.entityId == this.mc.thePlayer.entityId) {
			var2 = this.mc.thePlayer;
		}

		if(var2 != null) {
			((Entity)var2).mountEntity(var3);
		}
	}

	public void handleEntityStatus(Packet38EntityStatus var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null) {
			var2.handleHealthUpdate(var1.entityStatus);
		}

	}

	private Entity getEntityByID(int var1) {
		return (Entity)(var1 == this.mc.thePlayer.entityId ? this.mc.thePlayer : this.worldClient.getEntityByID(var1));
	}

	public void handleUpdateHealth(Packet8UpdateHealth var1) {
		this.mc.thePlayer.setHealth(var1.healthMP);
		this.mc.thePlayer.getFoodStats().setFoodLevel(var1.food);
		this.mc.thePlayer.getFoodStats().setFoodSaturationLevel(var1.foodSaturation);
	}

	public void handleExperience(Packet43Experience var1) {
		this.mc.thePlayer.setXPStats(var1.experience, var1.experienceTotal, var1.experienceLevel);
	}

	public void handleRespawn(Packet9Respawn var1) {
		if(var1.respawnDimension != this.mc.thePlayer.dimension) {
			this.field_1210_g = false;
			this.worldClient = new WorldClient(this, new WorldSettings(0L, var1.creativeMode, false, false, var1.terrainType), var1.respawnDimension, var1.difficulty);
			this.worldClient.isRemote = true;
			this.mc.changeWorld1(this.worldClient);
			this.mc.thePlayer.dimension = var1.respawnDimension;
			this.mc.displayGuiScreen(new GuiDownloadTerrain(this));
		}

		this.mc.respawn(true, var1.respawnDimension, false);
		((PlayerControllerMP)this.mc.playerController).setCreative(var1.creativeMode == 1);
	}

	public void handleExplosion(Packet60Explosion var1) {
		Explosion var2 = new Explosion(this.mc.theWorld, (Entity)null, var1.explosionX, var1.explosionY, var1.explosionZ, var1.explosionSize);
		var2.destroyedBlockPositions = var1.destroyedBlockPositions;
		var2.doExplosionB(true);
	}

	public void handleOpenWindow(Packet100OpenWindow var1) {
		EntityPlayerSP var2 = this.mc.thePlayer;
		switch(var1.inventoryType) {
		case 0:
			var2.displayGUIChest(new InventoryBasic(var1.windowTitle, var1.slotsCount));
			var2.craftingInventory.windowId = var1.windowId;
			break;
		case 1:
			var2.displayWorkbenchGUI(MathHelper.floor_double(var2.posX), MathHelper.floor_double(var2.posY), MathHelper.floor_double(var2.posZ));
			var2.craftingInventory.windowId = var1.windowId;
			break;
		case 2:
			var2.displayGUIFurnace(new TileEntityFurnace());
			var2.craftingInventory.windowId = var1.windowId;
			break;
		case 3:
			var2.displayGUIDispenser(new TileEntityDispenser());
			var2.craftingInventory.windowId = var1.windowId;
			break;
		case 4:
			var2.displayGUIEnchantment(MathHelper.floor_double(var2.posX), MathHelper.floor_double(var2.posY), MathHelper.floor_double(var2.posZ));
			var2.craftingInventory.windowId = var1.windowId;
			break;
		case 5:
			var2.displayGUIBrewingStand(new TileEntityBrewingStand());
			var2.craftingInventory.windowId = var1.windowId;
		}

	}

	public void handleSetSlot(Packet103SetSlot var1) {
		EntityPlayerSP var2 = this.mc.thePlayer;
		if(var1.windowId == -1) {
			var2.inventory.setItemStack(var1.myItemStack);
		} else if(var1.windowId == 0 && var1.itemSlot >= 36 && var1.itemSlot < 45) {
			ItemStack var3 = var2.inventorySlots.getSlot(var1.itemSlot).getStack();
			if(var1.myItemStack != null && (var3 == null || var3.stackSize < var1.myItemStack.stackSize)) {
				var1.myItemStack.animationsToGo = 5;
			}

			var2.inventorySlots.putStackInSlot(var1.itemSlot, var1.myItemStack);
		} else if(var1.windowId == var2.craftingInventory.windowId) {
			var2.craftingInventory.putStackInSlot(var1.itemSlot, var1.myItemStack);
		}

	}

	public void handleTransaction(Packet106Transaction var1) {
		Container var2 = null;
		EntityPlayerSP var3 = this.mc.thePlayer;
		if(var1.windowId == 0) {
			var2 = var3.inventorySlots;
		} else if(var1.windowId == var3.craftingInventory.windowId) {
			var2 = var3.craftingInventory;
		}

		if(var2 != null) {
			if(var1.accepted) {
				var2.func_20113_a(var1.shortWindowId);
			} else {
				var2.func_20110_b(var1.shortWindowId);
				this.addToSendQueue(new Packet106Transaction(var1.windowId, var1.shortWindowId, true));
			}
		}

	}

	public void handleWindowItems(Packet104WindowItems var1) {
		EntityPlayerSP var2 = this.mc.thePlayer;
		if(var1.windowId == 0) {
			var2.inventorySlots.putStacksInSlots(var1.itemStack);
		} else if(var1.windowId == var2.craftingInventory.windowId) {
			var2.craftingInventory.putStacksInSlots(var1.itemStack);
		}

	}

	public void handleUpdateSign(Packet130UpdateSign var1) {
		if(this.mc.theWorld.blockExists(var1.xPosition, var1.yPosition, var1.zPosition)) {
			TileEntity var2 = this.mc.theWorld.getBlockTileEntity(var1.xPosition, var1.yPosition, var1.zPosition);
			if(var2 instanceof TileEntitySign) {
				TileEntitySign var3 = (TileEntitySign)var2;
				if(var3.func_50007_a()) {
					for(int var4 = 0; var4 < 4; ++var4) {
						var3.signText[var4] = var1.signLines[var4];
					}

					var3.onInventoryChanged();
				}
			}
		}

	}

	public void handleTileEntityData(Packet132TileEntityData var1) {
		if(this.mc.theWorld.blockExists(var1.xPosition, var1.yPosition, var1.zPosition)) {
			TileEntity var2 = this.mc.theWorld.getBlockTileEntity(var1.xPosition, var1.yPosition, var1.zPosition);
			if(var2 != null && var1.actionType == 1 && var2 instanceof TileEntityMobSpawner) {
				((TileEntityMobSpawner)var2).setMobID(EntityList.getStringFromID(var1.customParam1));
			}
		}

	}

	public void handleUpdateProgressbar(Packet105UpdateProgressbar var1) {
		EntityPlayerSP var2 = this.mc.thePlayer;
		this.registerPacket(var1);
		if(var2.craftingInventory != null && var2.craftingInventory.windowId == var1.windowId) {
			var2.craftingInventory.updateProgressBar(var1.progressBar, var1.progressBarValue);
		}

	}

	public void handlePlayerInventory(Packet5PlayerInventory var1) {
		Entity var2 = this.getEntityByID(var1.entityID);
		if(var2 != null) {
			var2.outfitWithItem(var1.slot, var1.itemID, var1.itemDamage);
		}

	}

	public void handleCloseWindow(Packet101CloseWindow var1) {
		this.mc.thePlayer.closeScreen();
	}

	public void handlePlayNoteBlock(Packet54PlayNoteBlock var1) {
		this.mc.theWorld.playNoteAt(var1.xLocation, var1.yLocation, var1.zLocation, var1.instrumentType, var1.pitch);
	}

	public void handleBed(Packet70Bed var1) {
		EntityPlayerSP var2 = this.mc.thePlayer;
		int var3 = var1.bedState;
		if(var3 >= 0 && var3 < Packet70Bed.bedChat.length && Packet70Bed.bedChat[var3] != null) {
			var2.addChatMessage(Packet70Bed.bedChat[var3]);
		}

		if(var3 == 1) {
			this.worldClient.getWorldInfo().setRaining(true);
			this.worldClient.setRainStrength(1.0F);
		} else if(var3 == 2) {
			this.worldClient.getWorldInfo().setRaining(false);
			this.worldClient.setRainStrength(0.0F);
		} else if(var3 == 3) {
			((PlayerControllerMP)this.mc.playerController).setCreative(var1.gameMode == 1);
		} else if(var3 == 4) {
			this.mc.displayGuiScreen(new GuiWinGame());
		}

	}

	public void handleMapData(Packet131MapData var1) {
		if(var1.itemID == Item.map.shiftedIndex) {
			ItemMap.getMPMapData(var1.uniqueID, this.mc.theWorld).func_28171_a(var1.itemData);
		} else {
			System.out.println("Unknown itemid: " + var1.uniqueID);
		}

	}

	public void handleDoorChange(Packet61DoorChange var1) {
		this.mc.theWorld.playAuxSFX(var1.sfxID, var1.posX, var1.posY, var1.posZ, var1.auxData);
	}

	public void handleStatistic(Packet200Statistic var1) {
		((EntityClientPlayerMP)this.mc.thePlayer).incrementStat(StatList.getOneShotStat(var1.statisticId), var1.amount);
	}

	public void handleEntityEffect(Packet41EntityEffect var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null && var2 instanceof EntityLiving) {
			((EntityLiving)var2).addPotionEffect(new PotionEffect(var1.effectId, var1.duration, var1.effectAmp));
		}
	}

	public void handleRemoveEntityEffect(Packet42RemoveEntityEffect var1) {
		Entity var2 = this.getEntityByID(var1.entityId);
		if(var2 != null && var2 instanceof EntityLiving) {
			((EntityLiving)var2).removePotionEffect(var1.effectId);
		}
	}

	public boolean isServerHandler() {
		return false;
	}

	public void handlePlayerInfo(Packet201PlayerInfo var1) {
		GuiPlayerInfo var2 = (GuiPlayerInfo)this.playerInfoMap.get(var1.playerName);
		if(var2 == null && var1.isConnected) {
			var2 = new GuiPlayerInfo(var1.playerName);
			this.playerInfoMap.put(var1.playerName, var2);
			this.playerNames.add(var2);
		}

		if(var2 != null && !var1.isConnected) {
			this.playerInfoMap.remove(var1.playerName);
			this.playerNames.remove(var2);
		}

		if(var1.isConnected && var2 != null) {
			var2.responseTime = var1.ping;
		}

	}

	public void handleKeepAlive(Packet0KeepAlive var1) {
		this.addToSendQueue(new Packet0KeepAlive(var1.randomId));
	}

	public void func_50100_a(Packet202PlayerAbilities var1) {
		EntityPlayerSP var2 = this.mc.thePlayer;
		var2.capabilities.isFlying = var1.field_50070_b;
		var2.capabilities.isCreativeMode = var1.field_50069_d;
		var2.capabilities.disableDamage = var1.field_50072_a;
		var2.capabilities.allowFlying = var1.field_50071_c;
	}
}
