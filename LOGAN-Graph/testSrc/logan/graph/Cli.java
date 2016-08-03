package logan.graph;

import java.io.File;

public class Cli {

	public Cli() {
		// TODO Auto-generated constructor stub
	}

	public static void main(String[] args) throws Exception
	{
		DeBruijnGraphConfig config=new DeBruijnGraphConfig(23, 23);
		Graph graph=new Graph(config);
		
		TestHelper helper=new TestHelper(graph);
		
		int indexingThreadCount=Integer.parseInt(args[0]);
		int routingThreadCount=Integer.parseInt(args[1]);

		File file=new File(args[2]);
		
		helper.graphIndexingHelper(new File[] {file}, indexingThreadCount);

		graph.switchMode();
		
		helper.graphRoutingHelper(new File[] {file}, routingThreadCount);

		GraphWalker walker=new GraphWalker(graph);
		
		walker.dump();
		
		graph.free();
	}

}
