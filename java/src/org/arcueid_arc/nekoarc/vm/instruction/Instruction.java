package org.arcueid_arc.nekoarc.vm.instruction;

import org.arcueid_arc.nekoarc.NekoArcException;
import org.arcueid_arc.nekoarc.vm.VirtualMachine;

public interface Instruction {
	public void invoke(VirtualMachine vm) throws NekoArcException;
}
