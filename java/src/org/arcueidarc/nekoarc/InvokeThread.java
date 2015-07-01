package org.arcueidarc.nekoarc;

import java.util.concurrent.SynchronousQueue;

import org.arcueidarc.nekoarc.types.ArcObject;
import org.arcueidarc.nekoarc.types.Closure;
import org.arcueidarc.nekoarc.vm.VirtualMachine;

/** The main reason this class exists is that Java is a weak-sauce language that doesn't have closures */
public class InvokeThread implements Runnable
{
	public final SynchronousQueue<ArcObject> syncqueue;
	public final VirtualMachine vm;
	public final ArcObject obj;

	public InvokeThread(VirtualMachine vm, ArcObject obj)
	{
		syncqueue = new SynchronousQueue<ArcObject>();
		this.vm = vm;
		this.obj = obj;
	}

	@Override
	public void run()
	{
		put(obj.invoke(this));
	}

	public ArcObject getenv(int i, int j)
	{
		return(vm.getenv(i, j));
	}

	public void put(ArcObject val)
	{
		for (;;) {
			try {
				syncqueue.put(val);
				break;
			} catch (InterruptedException e) {}
		}
	}
	
	/** Synchronise invoke thread and virtual machine thread */
	public ArcObject sync()
	{
		for (;;) {
			try {
				return(syncqueue.take());
			} catch (InterruptedException e) {}
		}
	}

	/** Perform a call to some Arc object */
	public ArcObject apply(Closure fn, ArcObject...args)
	{
		// First, push all of the arguments to the stack
		for (ArcObject arg : args)
			vm.push(arg);
		// new continuation
		vm.setCont(new JavaContinuation(this, vm.getCont()));
		// apply the closure
		fn.apply(vm);
		// Create a continuation that will cause the vm to begin execution right then and there
		vm.makecont(0);
		// cause the virtual machine thread to begin executing
		put(fn);
		// suspend this invoke thread until the virtual machine restores the JavaContinuation
		return(sync());
	}
}
