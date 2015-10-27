package logan.graph;

//import pagan.dbg.DeBruijnException;
//import pagan.dbg.DeBruijnGraphConfig;
//import pagan.dbg.LinkedSmer;

public class Graph {

	static
	{
	System.loadLibrary("LOGAN");
	}

	//private DeBruijnGraphConfig config;

    private long graphHandle;

   //private int currentMode;

/*
 * Public API Begins
 */

	public Graph(int nodeSize, int edgeSize)
	{
//		this.config=config;
		graphHandle = alloc_Native(nodeSize,edgeSize);
	}

	/*
public DeBruijnGraphConfig getConfig()
{
	return config;
}
*/

	private native long alloc_Native(int nodeSize, int edgeSize);
	
	private native void free_Native(long handle);
	
	private native void dump_Native(long handle);

}
