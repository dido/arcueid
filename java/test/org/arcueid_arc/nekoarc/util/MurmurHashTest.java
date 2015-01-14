package org.arcueid_arc.nekoarc.util;

import static org.junit.Assert.*;

import org.junit.Test;

public class MurmurHashTest {

	@Test
	public void test()
	{
		assertEquals(0xe34bbc7bbc071b6cL, MurmurHash.hash("The quick brown fox jumps over the lazy dog", 0));
		assertEquals(0x658ca970ff85269aL, MurmurHash.hash("The quick brown fox jumps over the lazy cog", 0));
		assertEquals(0x738a7f3bd2633121L, MurmurHash.hash("The quick brown fox jumps over the lazy dog", 0x9747b28cL));
		assertEquals(0xb8cd57b070826194L, MurmurHash.hash("The quick brown fox jumps over the lazy cog", 0x9747b28cL));
	}
}
