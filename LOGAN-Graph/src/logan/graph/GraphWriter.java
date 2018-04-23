package logan.graph;

import java.io.File;

public class GraphWriter {
	private Graph graph;
	
	public GraphWriter(Graph graph)
	{
		this.graph=graph;
	}
	
	public long writeNodes(File file)
	{
		return writeNodes_Native(graph.getGraphHandle(), file.getAbsolutePath());
	}
	
	public long writeEdges(File file)
	{
		return writeEdges_Native(graph.getGraphHandle(), file.getAbsolutePath());
	}
	
	public long writeRoutes(File file)
	{
		return writeRoutes_Native(graph.getGraphHandle(), file.getAbsolutePath());
	}
	
	private native long writeNodes_Native(long graphHandle, String filePath);
	private native long writeEdges_Native(long graphHandle, String filePath);
	private native long writeRoutes_Native(long graphHandle, String filePath);
}
