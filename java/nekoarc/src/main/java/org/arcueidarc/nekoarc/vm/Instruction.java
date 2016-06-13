package org.arcueidarc.nekoarc.vm;

import org.arcueidarc.nekoarc.NekoArcException;

public interface Instruction {
	public void invoke(VirtualMachine vm) throws NekoArcException;
}
