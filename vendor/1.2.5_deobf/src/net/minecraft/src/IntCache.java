package net.minecraft.src;

import java.util.ArrayList;
import java.util.List;

public class IntCache {
	private static int intCacheSize = 256;
	private static List freeSmallArrays = new ArrayList();
	private static List inUseSmallArrays = new ArrayList();
	private static List freeLargeArrays = new ArrayList();
	private static List inUseLargeArrays = new ArrayList();

	public static int[] getIntCache(int var0) {
		int[] var1;
		if(var0 <= 256) {
			if(freeSmallArrays.size() == 0) {
				var1 = new int[256];
				inUseSmallArrays.add(var1);
				return var1;
			} else {
				var1 = (int[])freeSmallArrays.remove(freeSmallArrays.size() - 1);
				inUseSmallArrays.add(var1);
				return var1;
			}
		} else if(var0 > intCacheSize) {
			intCacheSize = var0;
			freeLargeArrays.clear();
			inUseLargeArrays.clear();
			var1 = new int[intCacheSize];
			inUseLargeArrays.add(var1);
			return var1;
		} else if(freeLargeArrays.size() == 0) {
			var1 = new int[intCacheSize];
			inUseLargeArrays.add(var1);
			return var1;
		} else {
			var1 = (int[])freeLargeArrays.remove(freeLargeArrays.size() - 1);
			inUseLargeArrays.add(var1);
			return var1;
		}
	}

	public static void resetIntCache() {
		if(freeLargeArrays.size() > 0) {
			freeLargeArrays.remove(freeLargeArrays.size() - 1);
		}

		if(freeSmallArrays.size() > 0) {
			freeSmallArrays.remove(freeSmallArrays.size() - 1);
		}

		freeLargeArrays.addAll(inUseLargeArrays);
		freeSmallArrays.addAll(inUseSmallArrays);
		inUseLargeArrays.clear();
		inUseSmallArrays.clear();
	}
}
