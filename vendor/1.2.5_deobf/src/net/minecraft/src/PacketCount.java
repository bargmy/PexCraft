package net.minecraft.src;

import java.util.HashMap;
import java.util.Map;

public class PacketCount {
	public static boolean allowCounting = true;
	private static final Map packetCountForID = new HashMap();
	private static final Map sizeCountForID = new HashMap();
	private static final Object lock = new Object();

	public static void countPacket(int var0, long var1) {
		if(allowCounting) {
			Object var3 = lock;
			synchronized(var3) {
				if(packetCountForID.containsKey(Integer.valueOf(var0))) {
					packetCountForID.put(Integer.valueOf(var0), Long.valueOf(((Long)packetCountForID.get(Integer.valueOf(var0))).longValue() + 1L));
					sizeCountForID.put(Integer.valueOf(var0), Long.valueOf(((Long)sizeCountForID.get(Integer.valueOf(var0))).longValue() + var1));
				} else {
					packetCountForID.put(Integer.valueOf(var0), Long.valueOf(1L));
					sizeCountForID.put(Integer.valueOf(var0), Long.valueOf(var1));
				}

			}
		}
	}
}
