package logan.graph;

public class DeBruijnGraphConfig {
	
	private int nodeSize;
	private int sparseness;
	
	public DeBruijnGraphConfig(int nodeSize, int sparseness)
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
