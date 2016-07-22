package logan.graph;

import java.io.File;
import java.io.IOException;

import junit.framework.TestCase;
import logan.graph.Graph.IndexBuilder;
import logan.graph.Graph.RouteBuilder;

public class GraphTest extends TestCase {

	Graph graph;
	TestHelper helper;

	protected void setUp() throws Exception {
		super.setUp();

		DeBruijnGraphConfig config=new DeBruijnGraphConfig(23, 23);
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
		//File files[]={new File("../data/Ecoli-1_Q20.fq")};

//		File files1[]={new File("../data/Single.fq")};
//		File files2[]={new File("../data/Single.fq")};
			
		File files1[]={new File("../data/Ecoli-1_Q20_10.fq")};
		File files2[]={new File("../data/Ecoli-1_Q20_10.fq")};

//		File files1[]={new File("../data/Arabi-1_Q20.fq")};
//		File files2[]={new File("../data/Arabi-1_Q20_10m.fq")};

		int threadCount=1;

		helper.graphIndexingHelper(files1,threadCount);
		System.out.println("Indexing complete");

		long smers[];

		System.out.println("Smer Count (after Index): "+graph.getSmerCount());
		smers=graph.getSmerIds();

		System.out.println("Smer Array Length: "+smers.length);
		if(smers.length<10)
			{
			for(long smer: smers)
				System.out.println(smer);
			}

		graph.switchMode();


		helper.graphRoutingHelper(files2,threadCount);
		System.out.println("Routing complete1");

//		graphRoutingHelper(files,threadCount);
//		System.out.println("Routing complete2");

		System.out.println("Smer Count (after Route): "+graph.getSmerCount());
		smers=graph.getSmerIds();
		System.out.println("Smer Array Length: "+smers.length);

		GraphWalker walker=new GraphWalker(graph);
		walker.dump();

		/*
		if(smers.length<10)
			{
			for(long smer: smers)
				System.out.println(smer);

			for(long smer: smers)
				{
				LinkedSmer linkedSmer=graph.getLinkedSmer(smer);
				System.out.println(linkedSmer);
				}

			}
			*/
		
			}
		catch(Exception e)
			{
			e.printStackTrace();
			throw e;
			}
	}



}
