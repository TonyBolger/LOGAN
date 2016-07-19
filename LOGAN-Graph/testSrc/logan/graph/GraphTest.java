package logan.graph;

import java.io.File;
import java.io.IOException;

import junit.framework.TestCase;
import logan.graph.Graph.IndexBuilder;
import logan.graph.Graph.RouteBuilder;

public class GraphTest extends TestCase {

	Graph graph;

	protected void setUp() throws Exception {
		super.setUp();

		DeBruijnGraphConfig config=new DeBruijnGraphConfig(23, 23);
		graph=new Graph(config);
	}

	protected void tearDown() throws Exception {
		super.tearDown();
	}

	/*
	public void testGraph()
	{

	}
	*/



	public void graphIndexingHelper(File files[], int threadCount) throws InterruptedException, IOException
	{
		final IndexBuilder ib=graph.makeIndexBuilder(threadCount);

		System.out.println("Indexing: Starting threads");
		Thread threads[]=new Thread[threadCount];
		for(int i=0;i<threads.length;i++)
			{
			threads[i]=new Thread(new Runnable() {
				@Override
				public void run() {
					System.out.println("Indexing: Worker started");
					ib.workerPerformTasks();
					System.out.println("Indexing: Worker stopped");
				}});

			threads[i].start();
			}


		System.out.println("Indexing: Waiting startup");
		ib.waitStartup();
		System.out.println("Indexing: Completed startup");

		System.out.println("Indexing: Ingress");
		ib.processReadFiles(files);

		System.out.println("Indexing: Shutdown");
		ib.shutdown();
		System.out.println("Indexing: Waiting shutdown");
		ib.waitShutdown();
		System.out.println("Indexing: Shutdown complete");

		System.out.println("Indexing: joining threads");
		for(int i=0;i<threads.length;i++)
			threads[i].join();
		System.out.println("Indexing: All threads joined");

		System.out.println("Indexing: Freeing");

		ib.free();
	}


	public void graphRoutingHelper(File files[], int threadCount) throws InterruptedException, IOException
	{
		final RouteBuilder rb=graph.makeRouteBuilder(threadCount);

		System.out.println("Routing: Starting threads");
		Thread threads[]=new Thread[threadCount];
		for(int i=0;i<threads.length;i++)
			{
			threads[i]=new Thread(new Runnable() {
				@Override
				public void run() {
					System.out.println("Routing: Worker started");
					rb.workerPerformTasks();
					System.out.println("Routing: Worker stopped");
				}});

			threads[i].start();
			}


		System.out.println("Routing: Waiting startup");
		rb.waitStartup();
		System.out.println("Routing: Completed startup");

		System.out.println("Routing: Ingress");
		rb.processReadFiles(files);

		System.out.println("Routing: Shutdown");
		rb.shutdown();
		System.out.println("Routing: Waiting shutdown");
		rb.waitShutdown();
		System.out.println("Routing: Shutdown complete");

		System.out.println("Routing: joining threads");
		for(int i=0;i<threads.length;i++)
			threads[i].join();
		System.out.println("Routing: All threads joined");

		System.out.println("Routing: Freeing");

		rb.free();
	}


	public void testIndexingAndRouting() throws Exception
	{
		try
			{
		//File files[]={new File("../data/Ecoli-1_Q20.fq")};
		//File files[]={new File("../data/Single.fq")};

			
//		File files1[]={new File("../data/Ecoli-1_Q20.fq")};
//		File files2[]={new File("../data/Ecoli-1_Q20.fq")};

		File files1[]={new File("../data/Arabi-1_Q20.fq")};
		File files2[]={new File("../data/Arabi-1_Q20.fq")};

		
		int threadCount=1;

		graphIndexingHelper(files1,threadCount);
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


		graphRoutingHelper(files2,threadCount);
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
