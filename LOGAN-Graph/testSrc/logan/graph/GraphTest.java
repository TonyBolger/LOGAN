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
	
	
	public void testGraph()
	{
		
	}
	
	
	public void testGraphIndexer() throws InterruptedException
	{
		int threadCount=4;
		
		final IndexBuilder ib=graph.makeIndexBuilder(threadCount);
		
		Thread threads[]=new Thread[threadCount];
		for(int i=0;i<threads.length;i++)
			{
			threads[i]=new Thread(new Runnable() {
				@Override
				public void run() {
					ib.perform();
				}});
			
			threads[i].start();
			}
		
		System.out.println("Main waiting startup");
		
		Thread.sleep(50);
		
		ib.waitStartup();
		
		System.out.println("Main Started");
		
		Thread.sleep(50);
		
		ib.shutdown();
		
		System.out.println("Main waiting shutdown");
		
		ib.waitShutdown();
		
		System.out.println("Main Shutdown");
		
	}

}
