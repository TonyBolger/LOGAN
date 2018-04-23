package logan.graph;

import java.io.File;
import junit.framework.TestCase;

public class GraphReaderTest extends TestCase {

	GraphReader graphReader;
	TestHelper helper;

	protected void setUp() throws Exception {
		super.setUp();

		GraphConfig config=new GraphConfig(23, 23);
		graphReader=new GraphReader(config);


	}

	protected void tearDown() throws Exception {
		super.tearDown();
	}


	public void testReader() throws Exception
	{
		try
			{
			//String graphBase="../../LOGAN-scratch/graphs/Arabi-1";
			String graphBase="../../LOGAN-scratch/graphs/Arabi-1_j";
						
			graphReader.readNodes(new File(graphBase+".nodes"));
			graphReader.readEdges(new File(graphBase+".edges"));
			graphReader.readRoutes(new File(graphBase+".routes"));
			
			Graph graph=graphReader.getGraph();
			System.out.println("Smer Count: "+graph.getSmerCount());

//			GraphWalker walker=new GraphWalker(graph);
//			walker.dump();

			graph.free();
			}
		catch(Exception e)
			{
			e.printStackTrace();
			throw e;
			}
	}



}
