package logan.graph;

import java.util.Collection;
import java.util.List;

import logan.graph.LinkedSmer.WhichSequenceDirection;

public class GraphWalker
{
	private Graph graph;
	private LinkedSmerCache cache;

	public GraphWalker(Graph graph)
	{
		this.graph=graph;
		this.cache=new LinkedSmerCache(graph);
	}

	
	public void followEdge(LinkedSmer.EdgeContext edge, StringBuilder sb, boolean loud)
	{
		//System.out.println("Following: "+edge);
		
		//LinkedSmer [ID: 2868259474793, Seq: AAGGCGTTCACGCCGCATCCGGC, 
		//LinkedSmer [ID: 11473037899172, Seq: AGGCGTTCACGCCGCATCCGGCA, 
		
		/*
		long smerId=edge.getSmer().getSmerId();
		
		if(smerId==11473037899172L)
			{
			System.out.println("Smer is "+edge.getSmer());
			System.out.println();
			}
		*/
	
		
		if(edge!=null)
			{
			edge.extractSequence(sb, WhichSequenceDirection.ORIGINAL,true);
			}
		
		int maxCount=1000;
		
		while(edge!=null && maxCount-->0)
			{
			edge=edge.transitionAcrossNode(WhichSequenceDirection.ORIGINAL);
			if(loud)
				{
				System.out.println("Across Node: "+edge);
				System.out.println();
				}
			
			edge.extractSequence(sb, WhichSequenceDirection.ORIGINAL, false);
			
			LinkedSmer linkingSmer=cache.getLinkedSmer(edge.getLinkingSmerId());
			if(loud)
				{
				System.out.println("Target: "+linkingSmer);
				System.out.println();
				}
			
			edge=edge.transitionToLinkingEdge(WhichSequenceDirection.ORIGINAL, linkingSmer);
			if(loud)
				{
				System.out.println();
				System.out.println("Linked to: "+edge);
				}
			}
	
		
	}
	
	public void dump()
	{
		long smerIds[]=graph.getSmerIds();
		int reads=0;
		int smerCount=0;
		
		for(long smerId: smerIds)
			{
			//System.out.println(smerId);

			LinkedSmer linkedSmer=cache.getLinkedSmer(smerId);
			linkedSmer.verifyUniqueTails();
			linkedSmer.verifyTailIndexes();
			
			//System.out.println(linkedSmer);
			
			Collection<LinkedSmer.EdgeContext> readContexts=LinkedSmer.makeSequenceEndTailContexts(linkedSmer, true,  false,  true,  true);
						
			reads+=readContexts.size();
			
			for(LinkedSmer.EdgeContext edge: readContexts)
				{
				try
					{
					StringBuilder sb=new StringBuilder();
					followEdge(edge,sb,false);
					
					System.out.println("SEQ: "+sb.toString());
					}
				catch(RuntimeException e)
					{
					throw e;
					//followEdge(edge,true);
					}
				}
		
			if(++smerCount%1000000==0)
				System.out.println("Processed "+smerCount+" smers with "+reads+" reads");
			}

		System.out.println("Dump complete");
		System.out.println("Processed "+smerCount+" smers with "+reads+" reads");

	}


	
}
