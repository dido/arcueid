package org.arcueidarc.nekoarc.util;

import java.io.UnsupportedEncodingException;

public class MurmurHash
{
	private static final String CHARSET = "UTF8";
	private static byte tmpdata[] = new byte[8];

	static class State {
		long h1;
		long h2;
		long k1;
		long k2;
		long c1;
		long c2;
	}
	
	private static final State state = new State();

	/**
	 * Hash a string, using default seed (hexadecimal pi, look I have nothing up my sleeve).
	 */
	public static long hash(final String str)
	{
		return(hash(str, 0x3243F6A8885A308DL));
	}

	/**
	 * Hash a string, given a seed value.
	 * @param str the string to hash
	 * @param seed the seed for the hash
	 */
	public static long hash(final String str, long seed)
	{
		final byte[] data;

		if (str == null)
			return(hash(0, seed));
		try {
			data = str.getBytes(CHARSET);
		} catch (UnsupportedEncodingException e) {
			throw new RuntimeException("unexpected exception: " + e.getMessage());
		}
		return(hash(data, seed));
	}

	/**
	 * Hash a long, given a seed value
	 */
	public static long hash(long val, long seed)
	{
		tmpdata[0] = (byte)(val & 0xff);
		tmpdata[1] = (byte)((val >> 8) & 0xff);
		tmpdata[2] = (byte)((val >> 16) & 0xff);
		tmpdata[3] = (byte)((val >> 24) & 0xff);
		tmpdata[4] = (byte)((val >> 32) & 0xff);
		tmpdata[5] = (byte)((val >> 40) & 0xff);
		tmpdata[6] = (byte)((val >> 48) & 0xff);
		tmpdata[7] = (byte)((val >> 56) & 0xff);
		return(hash(tmpdata, seed));
	}

	public static long getblock(byte[] data, int i)
	{
		return((((long) data[i + 0] & 0x00000000000000FFL) << 0)
				| (((long) data[i + 1] & 0x00000000000000FFL) << 8)
				| (((long) data[i + 2] & 0x00000000000000FFL) << 16)
				| (((long) data[i + 3] & 0x00000000000000FFL) << 24)
				| (((long) data[i + 4] & 0x00000000000000FFL) << 32)
				| (((long) data[i + 5] & 0x00000000000000FFL) << 40)
				| (((long) data[i + 6] & 0x00000000000000FFL) << 48)
				| (((long) data[i + 7] & 0x00000000000000FFL) << 56));
	}

	private static void bmix(State state)
	{
		state.k1 *= state.c1;
		state.k1 = (state.k1 << 31) | (state.k1 >>> (64 - 31));
		state.k1 *= state.c2;
		state.h1 ^= state.k1;

		state.h1 = (state.h1 << 27) | (state.h1 >>> (64 - 27));
		state.h1 += state.h2;
		state.h1 = state.h1*5 + 0x52dce729;

		state.k2 *= state.c2;
		state.k2 = (state.k2 << 33) | (state.k2 >>> (64 - 33));
		state.k2 *= state.c1;
		
		state.h2 ^= state.k2;
		state.h2 = (state.h2 << 31) | (state.h2 >>> (64 - 31));
		state.h2 += state.h1;
		state.h2 = state.h2 * 5 + 0x38495ab5;
	}

	private static long fmix(long k)
	{
		k ^= k >>> 33;
		k *= 0xff51afd7ed558ccdL;
		k ^= k >>> 33;
		k *= 0xc4ceb9fe1a85ec53L;
	    k ^= k >>> 33;
		return(k);
	}

	public static long hash(final byte[] data, long seed)
	{
		// state.h1 = 0x9368e53c2f6af274L ^ seed;
		// state.h2 = 0x586dcd208f7cd3fdL ^ seed;
		state.h1 = state.h2 = seed;

		state.c1 = 0x87c37b91114253d5L;
		state.c2 = 0x4cf5ad432745937fL;
	
		for (int i = 0; i < data.length / 16; i++) {
			state.k1 = getblock(data, i * 2 * 8);
			state.k2 = getblock(data, (i * 2 + 1) * 8);
			bmix(state);
		}

		state.k1 = state.k2 = 0;
		int tail = (data.length >>> 4) << 4;

		switch (data.length & 15) {
			case 15: state.k2 ^= (long) data[tail + 14] << 48;
			case 14: state.k2 ^= (long) data[tail + 13] << 40;
			case 13: state.k2 ^= (long) data[tail + 12] << 32;
			case 12: state.k2 ^= (long) data[tail + 11] << 24;
			case 11: state.k2 ^= (long) data[tail + 10] << 16;
			case 10: state.k2 ^= (long) data[tail + 9] << 8;
			case 9: state.k2 ^= (long) data[tail + 8] << 0;
			state.k2 *= state.c2;
			state.k2 = (state.k2 << 33) | (state.k2 >>> (64 - 33));
			state.k2 *= state.c1;
			state.h2 ^= state.k2;
			case 8: state.k1 ^= (long) data[tail + 7] << 56;
			case 7: state.k1 ^= (long) data[tail + 6] << 48;
			case 6: state.k1 ^= (long) data[tail + 5] << 40;
			case 5: state.k1 ^= (long) data[tail + 4] << 32;
			case 4: state.k1 ^= (long) data[tail + 3] << 24;
			case 3: state.k1 ^= (long) data[tail + 2] << 16;
			case 2: state.k1 ^= (long) data[tail + 1] << 8;
			case 1: state.k1 ^= (long) data[tail + 0] << 0;
			state.k1 *= state.c1;
			state.k1 = (state.k1 << 31) | (state.k1 >>> (64 - 31));
			state.k1 *= state.c2;
			state.h1 ^= state.k1;
		}

		state.h1 ^= data.length;
		state.h2 ^= data.length;

	    state.h1 += state.h2;
	    state.h2 += state.h1;

	    state.h1 = fmix(state.h1);
	    state.h2 = fmix(state.h2);

	    state.h1 += state.h2;
	    state.h2 += state.h1;

	    return(state.h1);
	}
}
