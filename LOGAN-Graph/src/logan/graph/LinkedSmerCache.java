package logan.graph;

import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicInteger;

import logan.graph.LinkedSmer.WhichSequenceDirection;


public class LinkedSmerCache {
	private Graph graph;
	private ConcurrentHashMap<Long, CacheEntry> cache;

	private ReferenceQueue<LinkedSmer> refQueue;

	private AtomicInteger cleanupTrigger;
	private AtomicInteger cleanupCounter;

	private AtomicInteger hits;
	private AtomicInteger misses;
	private AtomicInteger expired;

	public LinkedSmerCache(Graph graph)
	{
		this.graph=graph;

		cleanupTrigger=new AtomicInteger();
		cleanupCounter=new AtomicInteger();

		hits=new AtomicInteger();
		misses=new AtomicInteger();
		expired=new AtomicInteger();

		invalidate();
	}

	public void invalidate()
	{
		cache=new ConcurrentHashMap<Long, CacheEntry>();//1000000,0.75f,100);
		refQueue = new ReferenceQueue<LinkedSmer>();
	}


	private void cleanup()
	{
		cleanupCounter.incrementAndGet();
		CacheEntry entry = (CacheEntry) refQueue.poll();

		while(entry != null)
		   {
		   Long smerId = entry.getSmerId();
		   cache.remove(smerId);

		   entry = (CacheEntry) refQueue.poll();
		   }
	}

	private void considerCleanup()
	{
		if((cleanupTrigger.incrementAndGet() & 0xFFF) == 0)
			cleanup();
	}


	public LinkedSmer getLinkedSmer(long smerId)
	{
		WeakReference<LinkedSmer> ref=cache.get(smerId);

		if(ref!=null)
			{
			LinkedSmer ls=ref.get();
			if(ls!=null)
				{
				hits.incrementAndGet();
				return ls;
				}
			else
				{
				expired.incrementAndGet();
				considerCleanup();
				}
			}
		else
			considerCleanup();

		LinkedSmer linkedSmer=graph.getLinkedSmer(smerId, Graph.LINKED_SMER_ROUTE_UNLIMITED);

		if(linkedSmer==null)
			throw new RuntimeException("Null LinkedSmer for "+smerId);

		if(linkedSmer.getTotalRoutes()>10000)
			{
			int fwd=linkedSmer.getForwardRouteEntries()!=null?linkedSmer.getForwardRouteEntries().length:0;
			int rev=linkedSmer.getReverseRouteEntries()!=null?linkedSmer.getReverseRouteEntries().length:0;

			String smerStr=linkedSmer.getSequence(WhichSequenceDirection.ORIGINAL);

			System.out.println("LINKEDSMER "+smerStr+" "+smerId+" has "+linkedSmer.getTotalRoutes()+" Fwd: "+fwd+" Rev: "+rev);
			}

		CacheEntry entry=new CacheEntry(smerId, linkedSmer, refQueue);
		cache.put(smerId, entry);

		misses.incrementAndGet();

		return linkedSmer;
	}


	public void showStats()
	{
		System.out.println("LinkedSmerCache - Size: "+cache.size()+" Hits: "+hits.get()+" Misses: "+misses.get()+" Expired: "+expired.get()+" Cleanups: "+cleanupCounter.get());
	}


	private static class CacheEntry extends WeakReference<LinkedSmer>
	  {
	    private Long smerId;

	    CacheEntry(Long smerId, LinkedSmer linkedSmer, ReferenceQueue<? super LinkedSmer> refQueue)
	    {
	    	super(linkedSmer, refQueue);
	    	this.smerId = smerId;
	    }

	    private Long getSmerId()
	    {
	    	return smerId;
	    }
	  }


}
