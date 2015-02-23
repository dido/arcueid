package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public abstract class ArcObject
{
	public ArcObject car()
	{
		throw new NekoArcException("Can't take car of " + this.type());
	}

	public ArcObject cdr()
	{
		throw new NekoArcException("Can't take cdr of " + this.type());
	}

	public ArcObject scar(ArcObject ncar)
	{
		throw new NekoArcException("Can't set car of " + this.type());
	}

	public ArcObject sref(ArcObject value, ArcObject index)
	{
		throw new NekoArcException("Can't sref" + this + "( a " + this.type() + "), other args were " + value + ", " + index);
	}

	public ArcObject add(ArcObject other)
	{
		throw new NekoArcException("add not implemented for " + type() + " " + this);
	}

	public long len()
	{
		throw new NekoArcException("len: expects one string, list, or hash argument");
	}

	public abstract ArcObject type();

	public void invoke(VirtualMachine vm, Cons args)
	{
		throw new NekoArcException("Cannot invoke object of type " + type());
	}
}
