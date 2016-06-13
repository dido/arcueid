package org.arcueidarc.nekoarc;

@SuppressWarnings("serial")
public class InvalidInstructionException extends NekoArcException
{
	public InvalidInstructionException(int ip)
	{
		super("Invalid instruction at IP=" + String.format("%08x", ip));
	}
}
