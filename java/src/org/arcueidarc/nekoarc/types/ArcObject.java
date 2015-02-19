package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.NekoArcException;

public abstract class ArcObject
{
	public ArcObject car()
	{
		throw new NekoArcException("Can't take car of " + this);
	}

	public ArcObject cdr()
	{
		throw new NekoArcException("Can't take cdr of " + this);
	}

	public ArcObject add(ArcObject car)
	{
		throw new NekoArcException("add not implemented for " + type() + " " + this);
	}

	public abstract ArcObject type();
}
