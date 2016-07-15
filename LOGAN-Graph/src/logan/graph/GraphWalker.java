package logan.graph;

import java.util.Collection;
import java.util.List;

public class GraphWalker
{
	private Graph graph;
	private LinkedSmerCache cache;

	public GraphWalker(Graph graph)
	{
		this.graph=graph;
		this.cache=new LinkedSmerCache(graph);
	}

	public void dump()
	{
		long smerIds[]=graph.getSmerIds();

		for(long smerId: smerIds)
			{
			System.out.println(smerId);

			LinkedSmer linkedSmer=cache.getLinkedSmer(smerId);

			Collection<LinkedSmer.EdgeContext> readContexts=LinkedSmer.makeSequenceEndTailContexts(linkedSmer, true,  false,  true,  true);

			System.out.println("Got "+readContexts.size());
			}



	}

}
