package org.arcueidarc.nekoarc.vm.instruction;

import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

public interface Instruction {
	public void invoke(VirtualMachine vm) throws NekoArcException;
}
