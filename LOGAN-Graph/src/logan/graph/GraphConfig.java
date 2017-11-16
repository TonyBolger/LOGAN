package logan.graph;

public class GraphConfig {
	
	private int nodeSize;
	private int sparseness;
	
	public GraphConfig(int nodeSize, int sparseness)
	{
		this.nodeSize=nodeSize;
		this.sparseness=sparseness;
	}

	public int getNodeSize() {
		return nodeSize;
	}

	public int getSparseness() {
		return sparseness;
	}
	
}
