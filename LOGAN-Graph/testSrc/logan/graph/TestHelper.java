package logan.graph;

import java.io.File;
import java.io.IOException;

import logan.graph.Graph.IndexBuilder;
import logan.graph.Graph.RouteBuilder;

public class TestHelper {

	private Graph graph;

	public TestHelper(Graph graph) {
		this.graph=graph;
	}


	public void graphIndexingHelper(File files[], int threadCount) throws InterruptedException, IOException
	{
		final IndexBuilder ib=graph.makeIndexBuilder(threadCount);

		/*
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
*/

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

		/*
		System.out.println("Indexing: joining threads");
		for(int i=0;i<threads.length;i++)
			threads[i].join();
		System.out.println("Indexing: All threads joined");
*/

		System.out.println("Indexing: Freeing");

		ib.free();
	}


	public void graphRoutingHelper(File files[], int threadCount) throws InterruptedException, IOException
	{
		final RouteBuilder rb=graph.makeRouteBuilder(threadCount);

		/*
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
*/

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

		/*
		System.out.println("Routing: joining threads");
		for(int i=0;i<threads.length;i++)
			threads[i].join();
		System.out.println("Routing: All threads joined");
*/

		System.out.println("Routing: Freeing");

		rb.free();
	}

}
