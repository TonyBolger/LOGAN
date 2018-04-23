package logan.graph;

import java.io.File;


public class GraphReader {

	private Graph graph;
	
	public GraphReader(GraphConfig graphConfig)
	{
		this.graph=new Graph(graphConfig);
		this.graph.switchMode();
	}
	
	public GraphReader()
	{
		this(new GraphConfig());
	}
	
	
	public long readNodes(File file)
	{
		return readNodes_Native(graph.getGraphHandle(), file.getAbsolutePath());
	}
	
	public long readEdges(File file)
	{
		return readEdges_Native(graph.getGraphHandle(), file.getAbsolutePath());
	}
	
	public long readRoutes(File file)
	{
		return readRoutes_Native(graph.getGraphHandle(), file.getAbsolutePath());
	}
	
	public Graph getGraph()
	{
		return graph;
	}

	private native long readNodes_Native(long graphHandle, String filePath);
	private native long readEdges_Native(long graphHandle, String filePath);
	private native long readRoutes_Native(long graphHandle, String filePath);
	
}
