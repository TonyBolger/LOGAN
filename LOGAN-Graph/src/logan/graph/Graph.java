package logan.graph;

import java.io.File;
import java.io.IOException;


public class Graph {

	enum Mode {INDEX,ROUTE};


	static
	{
	System.loadLibrary("LOGAN");
	}

	private GraphConfig config;
    private long graphHandle;

    private SequenceIndex sequenceIndex;
    private Mode currentMode;

/*
 * Public API Begins
 */

	public Graph(GraphConfig config)
	{
		this.config=config;
		currentMode=Mode.INDEX;
		graphHandle = alloc_Native(config.getNodeSize(),config.getSparseness());
	}

	long getGraphHandle()
	{
		return graphHandle;
	}
	
	public GraphConfig getConfig()
	{
		return config;
	}

	public int getSmerCount()
	{
		checkHandleAndMode(null);

		return getSmerCount_Native(graphHandle);
	}

	public long[] getSmerIds()
	{
		checkHandleAndMode(null);

		return getSmerIds_Native(graphHandle);
	}

	public SequenceIndex getSequenceIndex() throws GraphException
	{
		checkHandleAndMode(null);

		if(sequenceIndex==null)
			sequenceIndex=getSequenceIndex_Native(graphHandle);

		return sequenceIndex;
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
		currentMode=Mode.ROUTE;
	}

	public static final long LINKED_SMER_ROUTE_UNLIMITED=Long.MAX_VALUE;

	public LinkedSmer getLinkedSmer(long smerId, long routeLimit) throws GraphException
	{
		checkHandleAndMode(Mode.ROUTE);

		return getLinkedSmer_Native(graphHandle, smerId, routeLimit);
	}

	public void free() throws GraphException
	{
		checkHandle();
		free_Native(graphHandle);

		graphHandle=0;
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

		if(expected !=null && currentMode!=expected)
			throw new GraphException("Graph " + this + " not in expected mode ("+expected+")");
	}

	/* Native Query (Both Modes) */

	private native int getSmerCount_Native(long graphHandle);
	private native long[] getSmerIds_Native(long graphHandle);

	private native SequenceIndex getSequenceIndex_Native(long builderHandle);

	/* Native Query (Map Mode) */

	//private native void getSmerPathCounts_Native(long graphHandle, long smerIds[], int smerPaths[]);

	/* Native Query (Array Mode) */

	private native LinkedSmer getLinkedSmer_Native(long graphHandle, long smerId, long routeLimit);

//	private native int SA_validateSmer_Native(long graphHandle, long smerId);

    private native byte[] getRawSmerData_Native(long graphHandle, long smerId);
//	private native byte[] getRawSmerPrefix_Native(long graphHandle, long smerId);
//	private native byte[] getRawSmerSuffix_Native(long graphHandle, long smerId);
//	private native byte[] getRawSmerRoutes_Native(long graphHandle, long smerId);

//	private native void SA_getSmerStats_Native(long graphHandle, int startIndex, int endIndex, long smerIds[],
//			int smerPrefixes[], int smerPrefixBases[], int smerSuffixes[], int smerSuffixBases[],
//			int smerRoutePrefixBits[], int smerRouteSuffixBits[], int smerRouteWidthBits[],
//			int smerRouteForwardEntries[], int smerRouteReverseEntries[], int smerRoutes[]);

	/* Native Update (Map Mode) */

	private native long makeIndexBuilder_Native(long graphHandle, int threads);
	private native void addRawSmers_Native(long graphHandle, long smerIds[]);

	/* Native Update (Array Mode) */

	private native long makeRouteBuilder_Native(long graphHandle, int threads);

	private native void setRawSmerData_Native(long graphHandle, long smerId, byte data[]);
//	private native void setRawSmerPrefix_Native(long graphHandle, long smerId, byte prefix[]);
//	private native void setRawSmerSuffix_Native(long graphHandle, long smerId, byte suffix[]);
//	private native void setRawSmerRoutes_Native(long graphHandle, long smerId, byte routes[]);

	/* Native Other */

//	private native void SA_getMemPoolStats_Native(long graphHandle,  int poolCount[], int itemCount[], int itemEmpty[], int itemSize[], long allocSize[]);

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

		public void free()
		{
			free_Native(builderHandle);
			builderHandle=0;
		}

		/*
		public void processReadData(byte data[], int offsets[], int chunkSize)
		{
			processReads_Native(builderHandle, data, offsets, chunkSize);
		}
	*/

		public void processReadFiles(File files[]) throws IOException
		{
			byte filePaths[][]=new byte[files.length][];

			for(int i=0;i<files.length;i++)
				filePaths[i]=files[i].getCanonicalPath().getBytes();

			processReadFiles_Native(builderHandle, filePaths);
		}

		public void shutdown()
		{
			shutdown_Native(builderHandle);
		}

		public void waitShutdown()
		{
			waitShutdown_Native(builderHandle);
		}

//		public void workerPerformTasks()
//		{
//			workerPerformTasks_Native(builderHandle);
//		}


		private native void waitStartup_Native(long builderHandle);

		//private native void processReadData_Native(long builderHandle, byte data[], int offsets[], int chunkSize);
		private native void processReadFiles_Native(long builderHandle, byte filePaths[][]);

		private native void shutdown_Native(long builderHandle);
		private native void waitShutdown_Native(long builderHandle);

		private native void workerPerformTasks_Native(long builderHandle);

		private native void free_Native(long builderHandle);
	}





	public class RouteBuilder {
		private long builderHandle;

		private RouteBuilder(long builderHandle)
		{
			this.builderHandle=builderHandle;
		}

		public void free()
		{
			free_Native(builderHandle);
			builderHandle=0;
		}

		public void waitStartup()
		{
			waitStartup_Native(builderHandle);
		}

		public void processReadFiles(File files[]) throws IOException
		{
			byte filePaths[][]=new byte[files.length][];

			for(int i=0;i<files.length;i++)
				filePaths[i]=files[i].getCanonicalPath().getBytes();

			processReadFiles_Native(builderHandle, filePaths);
		}

		public void shutdown()
		{
			shutdown_Native(builderHandle);
		}

		public void waitShutdown()
		{
			waitShutdown_Native(builderHandle);
		}

		private native void waitStartup_Native(long builderHandle);

		//private native void processReads_Native(long builderHandle, byte data[], int offsets[], int chunkSize);
		private native void processReadFiles_Native(long builderHandle, byte filePaths[][]);

		private native void shutdown_Native(long builderHandle);
		private native void waitShutdown_Native(long builderHandle);

		private native void free_Native(long builderHandle);
	}
}
