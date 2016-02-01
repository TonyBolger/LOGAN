package logan.graph;


public class Graph {

	enum Mode {INDEX,ROUTE};
		
		
	static
	{
	System.loadLibrary("LOGAN");
	}

	private DeBruijnGraphConfig config;
    private long graphHandle;
    
    private Mode currentMode;

/*
 * Public API Begins
 */

	public Graph(DeBruijnGraphConfig config)
	{
		this.config=config;
		currentMode=Mode.INDEX;
		graphHandle = alloc_Native(config.getNodeSize(),config.getSparseness());
	}

	public DeBruijnGraphConfig getConfig()
	{
		return config;
	}

	public IndexBuilder makeIndexBuilder(int threads) throws GraphException
	{
		checkHandleAndMode(Mode.INDEX);
		
		long builderHandle=makeIndexBuilder_Native(graphHandle, threads);
		
		return new IndexBuilder(builderHandle);
	}
	
	public RouteBuilder makeRouteBuilder(int threads) throws GraphException
	{
		checkHandleAndMode(Mode.ROUTE);

		long builderHandle=makeRouteBuilder_Native(graphHandle, threads);
		
		return new RouteBuilder(builderHandle);
	}
	
	
	public void switchMode() throws GraphException
	{
		checkHandleAndMode(Mode.INDEX);
		
		switchMode_Native(graphHandle);
	}
	
	
	public LinkedSmer getLinkedSmer(long smerId) throws GraphException
	{
		checkHandleAndMode(Mode.ROUTE);

		return null;
		//return SA_getLinkedSmer_Native(graphHandle, smerId);
	}

	/* Sanity checks */
	
	private void checkHandle() throws GraphException
	{
		if (graphHandle == 0)
			throw new GraphException("Graph " + this + " has a null native handle");
	}
	
	private void checkHandleAndMode(Mode expected) throws GraphException
	{
		if (graphHandle == 0)
			throw new GraphException("Graph " + this + " has a null native handle");
		
		if(currentMode!=expected)
			throw new GraphException("Graph " + this + " not in expected mode ("+expected+")");
	}
	
	
	
	/* Native Query */
	
	
	/* Native Update */
	
	private native long makeIndexBuilder_Native(long graphHandle, int threads);
	private native long makeRouteBuilder_Native(long graphHandle, int threads);
		
	
	/* Native Other */
	
	private native void switchMode_Native(long graphHandle);
	
	private native long alloc_Native(int nodeSize, int sparseness);
	private native void free_Native(long graphHandle);

	private native void dump_Native(long graphHandle);

	
	/* Helpers */
	
	public class IndexBuilder {
		private long builderHandle;
		
		private IndexBuilder(long builderHandle)
		{
			this.builderHandle=builderHandle;
		}
		
		public void waitStartup()
		{
			waitStartup_Native(builderHandle);
		}

		public void processReads(byte data[], int offsets[], int chunkSize)
		{
			processReads_Native(builderHandle, data, offsets, chunkSize);
		}
	
		public void shutdown()
		{
			shutdown_Native(builderHandle);
		}

		public void waitShutdown()
		{
			waitShutdown_Native(builderHandle);
		}

		public void perform()
		{
			perform_Native(builderHandle);
		}
		
		
		private native void waitStartup_Native(long builderHandle);
		
		private native void processReads_Native(long builderHandle, byte data[], int offsets[], int chunkSize);  
		
		private native void shutdown_Native(long builderHandle);
		private native void waitShutdown_Native(long builderHandle);
		
		private native void perform_Native(long builderHandle);
		
		
	}
	

	
	
	
	public class RouteBuilder {
		private long builderHandle;
		
		private RouteBuilder(long builderHandle)
		{
			this.builderHandle=builderHandle;
		}
		
		public void shutdown()
		{
			this.builderHandle=0;
		}
		
		private native void waitStartup_Native(long builderHandle);
		
		private native void processReads_Native(long builderHandle, byte data[], int offsets[], int chunkSize);  
		
		private native void shutdown_Native(long builderHandle);
		private native void waitShutdown_Native(long builderHandle);
		
		private native void perform_Native(long builderHandle);
	}
}
