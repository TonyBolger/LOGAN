package logan.graph;

import java.io.File;

public class Cli {

	public Cli() {
		// TODO Auto-generated constructor stub
	}

	public static void main(String[] args) throws Exception
	{
		GraphConfig config=new GraphConfig(23, 23);
		Graph graph=new Graph(config);

		TestHelper helper=new TestHelper(graph);

		File file=new File(args[0]);

		int indexingThreadCount=Integer.parseInt(args[1]);
		int routingThreadCount=Integer.parseInt(args[2]);

		helper.graphIndexingHelper(new File[] {file}, indexingThreadCount);

		graph.switchMode();

		helper.graphRoutingHelper(new File[] {file}, routingThreadCount);

		GraphWalker walker=new GraphWalker(graph);

		walker.dump();

		graph.free();
	}

}
