package org.arcueidarc.nekoarc.types;

public abstract class Numeric extends Atom
{
	public abstract Numeric negate();
	public abstract Numeric mul(Numeric factor);
	public abstract Numeric div(Numeric divisor);
}
