package org.arcueidarc.nekoarc.functions;

import org.arcueidarc.nekoarc.HeapContinuation;
import org.arcueidarc.nekoarc.InvokeThread;
import org.arcueidarc.nekoarc.NekoArcException;
import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Fixnum;

public class CCC extends Builtin
{
	protected CCC()
	{
		super("ccc", 1);
	}

	@Override
	public ArcObject invoke(InvokeThread thr)
	{
		ArcObject continuation = thr.vm.getCont();
		if (continuation instanceof Fixnum)
			continuation = HeapContinuation.fromStackCont(thr.vm, continuation);
		if (!(continuation instanceof HeapContinuation))
			throw new NekoArcException("Invalid continuation type " + continuation.type().toString());
		return(thr.apply(thr.getenv(0,  0), continuation));
	}

}
