package org.arcueidarc.nekoarc.types;

import org.arcueidarc.nekoarc.InvokeThread;
import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.Nil;

public class Cons extends ArcObject
{
	public static final ArcObject TYPE = Symbol.intern("cons");
	private ArcObject car;
	private ArcObject cdr;

	public Cons()
	{
	}

	public Cons(ArcObject car, ArcObject cdr)
	{
		this.car = car;
		this.cdr = cdr;
	}

	@Override
	public ArcObject type()
	{
		return(TYPE);
	}

	@Override
	public ArcObject car()
	{
		return(car);
	}

	@Override
	public ArcObject cdr()
	{
		return(cdr);
	}

	@Override
	public ArcObject scar(ArcObject ncar)
	{
		this.car = ncar;
		return(ncar);
	}

	@Override
	public ArcObject scdr(ArcObject ncdr)
	{
		this.cdr = ncdr;
		return(ncdr);
	}
	
	@Override
	public ArcObject sref(ArcObject value, ArcObject idx)
	{
		Fixnum index = Fixnum.cast(idx, this);
		return(nth(index.fixnum).scar(value));
	}

	@SuppressWarnings("serial")
	static class OOB extends RuntimeException {
	}

	public Cons nth(long idx)
	{
		try {
			return((Cons)nth(this, idx));
		} catch (OOB oob) {
			throw new NekoArcException("Error: index " + idx + " too large for list " + this);
		}
	}

	private static ArcObject nth(ArcObject c, long idx)
	{
		while (idx > 0) {
			if (c.cdr() instanceof Nil)
				throw new OOB();
			c = c.cdr();
			idx--;
		}
		return(c);
	}

	@Override
	public int requiredArgs()
	{
		return(1);
	}

	@Override
	public ArcObject invoke(InvokeThread thr)
	{
		Fixnum idx = Fixnum.cast(thr.getenv(0, 0), this);
		return(this.nth(idx.fixnum).car());
	}
}

