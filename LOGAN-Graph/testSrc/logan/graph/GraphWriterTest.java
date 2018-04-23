package logan.graph;

import java.io.File;
import junit.framework.TestCase;

public class GraphWriterTest extends TestCase {

	Graph graph;
	TestHelper helper;

	protected void setUp() throws Exception {
		super.setUp();

		GraphConfig config=new GraphConfig(23, 23);
		graph=new Graph(config);

		helper=new TestHelper(graph);

	}

	protected void tearDown() throws Exception {
		super.tearDown();
	}

	/*
	public void testGraph()
	{

	}
	*/






	public void testIndexingAndRouting() throws Exception
	{
		try
			{
			File files[]={new File("../../LOGAN-scratch/data/Arabi-1_Q20.fq")};
			String graphBase="../../LOGAN-scratch/graphs/Arabi-1_j";
			
			int threadCount=8;

			helper.graphIndexingHelper(files,threadCount);
			System.out.println("Smer Count (after Index): "+graph.getSmerCount());

			graph.switchMode();

			helper.graphRoutingHelper(files,threadCount);
			System.out.println("Routing complete");

			GraphWriter graphWriter=new GraphWriter(graph);
			
			graphWriter.writeNodes(new File(graphBase+".nodes"));
			graphWriter.writeEdges(new File(graphBase+".edges"));
			graphWriter.writeRoutes(new File(graphBase+".routes"));

		
			graph.free();
			}
		catch(Exception e)
			{
			e.printStackTrace();
			throw e;
			}
	}



}
