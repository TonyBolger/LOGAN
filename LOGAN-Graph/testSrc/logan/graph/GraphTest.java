package logan.graph;

import junit.framework.TestCase;
import logan.graph.Graph.IndexBuilder;

public class GraphTest extends TestCase {

	Graph graph;
	
	protected void setUp() throws Exception {
		super.setUp();
		
		DeBruijnGraphConfig config=new DeBruijnGraphConfig(10, 20);
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
	
	public void testGraphIndexer() throws InterruptedException
	{
		int threadCount=4;
		
		Thread.sleep(50);
		System.out.println("Starting threads");
		Thread.sleep(50);
		
		
		final IndexBuilder ib=graph.makeIndexBuilder(threadCount);
		
		Thread threads[]=new Thread[threadCount];
		for(int i=0;i<threads.length;i++)
			{
			threads[i]=new Thread(new Runnable() {
				@Override
				public void run() {
					System.out.println("Java Worker started");
					
					ib.perform();
					
					System.out.println("Java Worker stopped");
				}});
			
			threads[i].start();
			}
		
		
		System.out.println("Main waiting startup");
		
		Thread.sleep(50);
		
		ib.waitStartup();
		
		System.out.println("Main Started");
		
		Thread.sleep(250);
		
		System.out.println("Main Ingress");
		
		ib.processReads(null, null, 20);
		
		Thread.sleep(10000);
		
		System.out.println("Main Ingress2");
		
		ib.processReads(null, null, 20);
		
		System.out.println("Main shutdown");
		
		ib.shutdown();
		
		System.out.println("Main waiting shutdown");
		
		ib.waitShutdown();
		
		System.out.println("Main Shutdown complete");
		
		Thread.sleep(250);
		
		System.out.println("Joining threads");
		
		for(int i=0;i<threads.length;i++)
			threads[i].join();
		
		System.out.println("All Joined");
		
	}

}
