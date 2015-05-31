package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;

public abstract class Env extends ArcObject
{
	public abstract Env nextEnv();
	public abstract ArcObject getEnv(int index);
}
