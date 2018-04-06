package logan.graph;

import java.io.File;
import java.io.IOException;

import junit.framework.TestCase;
import logan.analysis.linear.LinearCompactGraph;
import logan.analysis.wavefront.WavefrontAnalysis;
import logan.graph.Graph.IndexBuilder;
import logan.graph.Graph.RouteBuilder;

public class GraphTest extends TestCase {

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
		//	File files[]={new File("../data/Ecoli-1_Q20.fq")};
//			File files[]={new File("../data/Single.fq")};
//			File files[]={new File("../../LOGAN-scratch/data/Ecoli-1_Q20.fq")};
//			File files[]={new File("../../LOGAN-scratch/data/Test_ACGT.fq")};
//			File files[]={new File("../data/Arabi-1_Q20.fq")};
//			File files[]={new File("../../LOGAN-scratch/ref/TAIR10.fa"),new File("../../LOGAN-scratch/data/Arabi-1_Q20.fq")};
//			File files[]={new File("../../LOGAN-scratch/ref/TAIR10.fa")};
//			File files[]={new File("../../LOGAN-scratch/ref/SpennV200-chr.fa")};
			File files[]={new File("../../LOGAN-scratch/ref/SpennV200-chr.fa"),new File("../../LOGAN-scratch/ref/Spenn_Lost.fa")};
//			File files[]={new File("../../LOGAN-scratch/ref/hg38.fa")};

			int threadCount=8;

			helper.graphIndexingHelper(files,threadCount);
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

			helper.graphRoutingHelper(files,threadCount);
			System.out.println("Routing complete1");

//			graphRoutingHelper(files,threadCount);
//			System.out.println("Routing complete2");

			System.out.println("Smer Count (after Route): "+graph.getSmerCount());
			smers=graph.getSmerIds();
			System.out.println("Smer Array Length: "+smers.length);

//			graph.getSequenceIndex().dumpShallow();

//			GraphWalker walker=new GraphWalker(graph);
//			walker.dump();

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

/*			long totalRoutes=0;
			for(long smer: smers)
				{
				LinkedSmer linkedSmer=graph.getLinkedSmer(smer);
				totalRoutes+=(linkedSmer.getForwardRouteEntries().length+linkedSmer.getReverseRouteEntries().length);
				}
			System.out.println("Total Routes: "+totalRoutes);
*/

			graph.free();
			}
		catch(Exception e)
			{
			e.printStackTrace();
			throw e;
			}
	}



}
