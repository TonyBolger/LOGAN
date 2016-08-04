package logan.graph;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.List;
import java.util.ListIterator;

import logan.graph.LinkedSmer.WhichSequenceDirection;

public class GraphWalker {
	private Graph graph;
	private LinkedSmerCache cache;

	public GraphWalker(Graph graph) {
		this.graph = graph;
		this.cache = new LinkedSmerCache(graph);
	}

	public void followUpstream(LinkedSmer.EdgeContext startEdge, StringBuilder sb) {
		List<LinkedSmer.EdgeContext> edgeList = new ArrayList<LinkedSmer.EdgeContext>();

		LinkedSmer.EdgeContext edge = startEdge;
		// edgeList.add(edge);

		int maxCount = 1000;

		while (edge != null && maxCount-- > 0) {
			LinkedSmer linkingSmer = cache.getLinkedSmer(edge.getLinkingSmerId());

			edge = edge.transitionToLinkingEdge(WhichSequenceDirection.REVERSECOMP, linkingSmer);

			if (edge != null) {
				edge = edge.transitionAcrossNode(WhichSequenceDirection.REVERSECOMP);
				edgeList.add(edge);
			}
		}

		ListIterator<LinkedSmer.EdgeContext> iter = edgeList.listIterator(edgeList.size());

		if (iter.hasPrevious()) {
			edge = iter.previous();
			edge.extractSequence(sb, WhichSequenceDirection.ORIGINAL, true);

			while (iter.hasPrevious()) {
				edge = iter.previous();
				sb.append("-");
				edge.extractSequence(sb, WhichSequenceDirection.ORIGINAL, false);
			}
		}

		sb.append("-");
		startEdge.extractSequence(sb, WhichSequenceDirection.ORIGINAL, true);
	}

	public void followEdgeOnce(LinkedSmer.EdgeContext edge, StringBuilder sb)
	{
		edge = edge.transitionAcrossNode(WhichSequenceDirection.ORIGINAL);
		edge.extractSequence(sb, WhichSequenceDirection.ORIGINAL, false);
	}

	public void followEdge(LinkedSmer.EdgeContext edge, StringBuilder sb, boolean loud) {
		// System.out.println("Following: "+edge);

		// LinkedSmer [ID: 2868259474793, Seq: AAGGCGTTCACGCCGCATCCGGC,
		// LinkedSmer [ID: 11473037899172, Seq: AGGCGTTCACGCCGCATCCGGCA,

		/*
		 * long smerId=edge.getSmer().getSmerId();
		 *
		 * if(smerId==11473037899172L) { System.out.println("Smer is "
		 * +edge.getSmer()); System.out.println(); }
		 */

		if (edge != null) {
			edge.extractSequence(sb, WhichSequenceDirection.ORIGINAL, true);
		}

		int maxCount = 1000;

		while (edge != null && maxCount-- > 0) {
			edge = edge.transitionAcrossNode(WhichSequenceDirection.ORIGINAL);
			if (loud) {
				System.out.println("Across Node: " + edge);
				System.out.println();
			}

			edge.extractSequence(sb, WhichSequenceDirection.ORIGINAL, false);

			LinkedSmer linkingSmer = cache.getLinkedSmer(edge.getLinkingSmerId());
			if (loud) {
				System.out.println("Target: " + linkingSmer);
				System.out.println();
			}

			edge = edge.transitionToLinkingEdge(WhichSequenceDirection.ORIGINAL, linkingSmer);
			if (loud) {
				System.out.println();
				System.out.println("Linked to: " + edge);
			}
		}

	}

	public void verifyUpstream(LinkedSmer smer) {
		System.out.println(smer.toString());

		List<LinkedSmer.EdgeContext> forwardEdges = LinkedSmer.makeSequenceContexts(smer, true, false);

		System.out.println("ForwardUpstreams for smer: " + smer.getSequence(WhichSequenceDirection.ORIGINAL));

		for (LinkedSmer.EdgeContext forwardEdge : forwardEdges) {
			StringBuilder sb = new StringBuilder();
			followUpstream(forwardEdge, sb);

			while (sb.length() < 200)
				sb.insert(0, "          ");

			while (sb.length() < 210)
				sb.insert(0, " ");

			sb.append("-");
			followEdgeOnce(forwardEdge, sb);

			System.out.println("Forward: " + sb.toString());
		}

		List<LinkedSmer.EdgeContext> reverseEdges = LinkedSmer.makeSequenceContexts(smer, false, true);

		if (reverseEdges.size() > 1000) {
			System.out.println("ReverseUpstreams for smer: " + smer.getSequence(WhichSequenceDirection.ORIGINAL));

			for (LinkedSmer.EdgeContext reverseEdge : reverseEdges) {
				StringBuilder sb = new StringBuilder();
				followUpstream(reverseEdge, sb);

				while (sb.length() < 200)
					sb.insert(0, "          ");

				while (sb.length() < 210)
					sb.insert(0, " ");

				sb.append("-");
				followEdgeOnce(reverseEdge, sb);

				System.out.println("Reverse: " + sb.toString());
			}
		}

	}

	public void dump() {
		long smerIds[] = graph.getSmerIds();
		int reads = 0;
		int smerCount = 0;

		System.out.println("Dump begins");

		for (long smerId : smerIds) {
			// System.out.println(smerId);

			LinkedSmer linkedSmer = cache.getLinkedSmer(smerId);
			linkedSmer.verifyUniqueTails();
			linkedSmer.verifyTailIndexes();

			//if(linkedSmer.getSmerId()==0)
			//	verifyUpstream(linkedSmer);

			// System.out.println(linkedSmer);

			Collection<LinkedSmer.EdgeContext> readContexts = LinkedSmer.makeSequenceEndTailContexts(linkedSmer, true,
					false, true, true);

			reads += readContexts.size();

			for (LinkedSmer.EdgeContext edge : readContexts) {
				try {
					StringBuilder sb = new StringBuilder();
					followEdge(edge, sb, false);

					System.out.println("SEQ: "+sb.toString());
				} catch (RuntimeException e) {
					throw e;
					// followEdge(edge,true);
				}
			}

			if (++smerCount % 1000000 == 0)
				System.out.println("Processed " + smerCount + " smers with " + reads + " reads");
		}

		System.out.println("Dump complete");
		System.out.println("Processed " + smerCount + " smers with " + reads + " reads");

	}

}
