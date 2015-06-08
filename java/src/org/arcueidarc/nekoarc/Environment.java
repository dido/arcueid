package org.arcueidarc.nekoarc;

import org.arcueidarc.nekoarc.types.ArcObject;

public interface Environment
{
	public Environment prevEnv();
	public ArcObject getEnv(int index) throws NekoArcException;
	public ArcObject setEnv(int index, ArcObject value);
}
