package logan.graph;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class LinkedSmer {

	private final static int SMER_LENGTH=23;

	private final static char CANONICAL_BASES[]={'A','C','G','T'};
	private final static char COMPLEMENT_BASES[]={'T','G','C','A'};

	public static enum WhichTail
	{
		PREFIX(), SUFFIX();

		static WhichTail other(WhichTail tail)
		{
			return tail==PREFIX?SUFFIX:PREFIX;
		}
	}

	public static enum WhichRouteTable
	{
		FORWARD(), REVERSE();

		static WhichRouteTable other(WhichRouteTable routeTable)
		{
			return routeTable==FORWARD?REVERSE:FORWARD;
		}

	}

	public static enum WhichSequenceDirection
	{
		ORIGINAL(), REVERSECOMP();

		static WhichSequenceDirection other(WhichSequenceDirection direction)
		{
			return direction==ORIGINAL?REVERSECOMP:ORIGINAL;
		}
	}


	private long smerId;
	private Tail prefixes[];
	private Tail suffixes[];
	private RouteEntry forwardRouteEntries[];
	private RouteEntry reverseRouteEntries[];
	private RouteTag forwardRouteTags[];
	private RouteTag reverseRouteTags[];
	private long totalRoutes;

	public LinkedSmer(long smerId, Tail prefixes[], Tail suffixes[], RouteEntry forwardRouteEntries[], RouteEntry reverseRouteEntries[],
			RouteTag forwardRouteTags[], RouteTag reverseRouteTags[], long totalRoutes)
	{
		this.smerId=smerId;
		this.prefixes=prefixes;
		this.suffixes=suffixes;
		this.forwardRouteEntries=forwardRouteEntries;
		this.reverseRouteEntries=reverseRouteEntries;
		this.forwardRouteTags=forwardRouteTags;
		this.reverseRouteTags=reverseRouteTags;
		this.totalRoutes=totalRoutes;
	}

	public long getSmerId() {
		return smerId;
	}

	public Tail[] getPrefixes() {
		return prefixes;
	}

	public Tail[] getSuffixes() {
		return suffixes;
	}

	public RouteEntry[] getForwardRouteEntries() {
		return forwardRouteEntries;
	}

	public RouteEntry[] getReverseRouteEntries() {
		return reverseRouteEntries;
	}

	public RouteTag[] getForwardRouteTags() {
		return forwardRouteTags;
	}

	public RouteTag[] getReverseRouteTags() {
		return reverseRouteTags;
	}

	public long getTotalRoutes() {
		return totalRoutes;
	}

	public String toString()
	{
		StringBuilder sb=new StringBuilder();
		sb.append("LinkedSmer [ID: "+smerId);
		sb.append(", Seq: ");
		extractSequence(sb, WhichSequenceDirection.ORIGINAL);
		sb.append(", \nPrefixes("+prefixes.length+"): [");
		for(Tail p: prefixes)
			sb.append(p.toString());
		sb.append("], \nSuffixes("+suffixes.length+"): [");
		for(Tail s: suffixes)
			sb.append(s.toString());
		sb.append("], \nForwardRouteEntries("+forwardRouteEntries.length+"): [");
		for(RouteEntry r: forwardRouteEntries)
			sb.append(r.toString());
		sb.append("], \nReverseRouteEntries("+reverseRouteEntries.length+"): [");
		for(RouteEntry r: reverseRouteEntries)
			sb.append(r.toString());
		sb.append("], \nForwardRouteTags("+forwardRouteTags.length+"): [");
		for(RouteTag r: forwardRouteTags)
			sb.append(r.toString());
		sb.append("], \nReverseRouteTags("+reverseRouteTags.length+"): [");
		for(RouteTag r: reverseRouteTags)
			sb.append(r.toString());
		sb.append("] ]");


		return sb.toString();
	}

	public static char complementChar(char ch)
	{
		switch (ch)
			{
			case 'A':
				return 'T';
			case 'C':
				return 'G';
			case 'G':
				return 'C';
			case 'T':
				return 'A';
			}

		return 'N';
	}

	public static String complementSequence(String sequence)
	{
		StringBuilder sb=new StringBuilder(sequence);

		int len=sequence.length();
		int last=len-1;

		for(int i=0;i<sequence.length();i++)
			sb.setCharAt(i, complementChar(sequence.charAt(last-i)));

		return sb.toString();
	}


	private static void extractPackedSequence(long data, int length, StringBuilder sb, WhichSequenceDirection direction)
	{
		if(direction==WhichSequenceDirection.ORIGINAL)
			{
			for (int i = length-1; i >= 0; i--)
				{
				int base = (int)(data >> 2 * i) & 0x3;
				sb.append(CANONICAL_BASES[base]);
				}
			}
		else
			{
			for (int i = 0; i<length; i++)
				{
				int base = (int)(data >> 2 * i) & 0x3;
				sb.append(COMPLEMENT_BASES[base]);
				}
			}
	}

	private void extractSequence(StringBuilder sb, WhichSequenceDirection direction)
	{
		extractPackedSequence(smerId, SMER_LENGTH, sb, direction);
	}

	public String getSequence(WhichSequenceDirection direction)
	{
		StringBuilder sb=new StringBuilder();
		extractSequence(sb,direction);
		return sb.toString();
	}

/*
	public boolean verifyRouteOrder()
	{
		// Order should be by upstream, then by downstream

		if(forwardRoutes!=null && forwardRoutes.length>0)
			{
			int lastUpstream=forwardRoutes[0].prefix;
			int lastDownstream=forwardRoutes[0].suffix;
			int lastWidth=forwardRoutes[0].width;

			for(int i=1;i<forwardRoutes.length;i++)
				{
				int upstream=forwardRoutes[i].prefix;
				int downstream=forwardRoutes[i].suffix;
				int width=forwardRoutes[i].width;

				if(upstream<lastUpstream)
					{
					System.out.println("Node with F: "+forwardRoutes.length+" R: "+reverseRoutes.length+
							" Invalid order at "+i+" in forward: Previous "+lastUpstream+","+lastDownstream+" ("+lastWidth+") Current "+upstream+","+downstream+" ("+width+")");
					return false;
					}

				lastWidth=width;
				lastUpstream=upstream;
				lastDownstream=downstream;
				}
			}

		if(reverseRoutes!=null && reverseRoutes.length>0)
			{
			int lastUpstream=reverseRoutes[0].suffix;
			int lastDownstream=reverseRoutes[0].prefix;
			int lastWidth=reverseRoutes[0].width;

			for(int i=0;i<reverseRoutes.length;i--)
				{
				int upstream=reverseRoutes[i].suffix;
				int downstream=reverseRoutes[i].prefix;
				int width=reverseRoutes[i].width;

				if(upstream<lastUpstream)
					{
					System.out.println("Node with F: "+forwardRoutes.length+" R: "+reverseRoutes.length+
							" Invalid order at "+i+" in reverse: Previous "+lastUpstream+","+lastDownstream+" ("+lastWidth+") Current "+upstream+","+downstream+" ("+width+")");
					return false;
					}

				lastWidth=width;
				lastUpstream=upstream;
				lastDownstream=downstream;
				}
			}

		return true;
	}

*/

	// Which prefix of this smer links to the given smer-tailData combo

	// Prefix     <- this
	// SmerId -> tailData

	public static long TAIL_LENGTH_MASK=0xFFFF000000000000L;
	public static int TAIL_LENGTH_SHIFT=48;
	public static long TAIL_BASES_MASK=0x0000FFFFFFFFFFFFL;

	private int prefixSmerIndex(long smerId, byte exists, long tailData)
	{
		int count=0,val=0;

		long maskedTail=tailData & TAIL_LENGTH_MASK;

		for(int i=0;i<prefixes.length;i++)
			if(prefixes[i].smerExists==exists && prefixes[i].smerId==smerId && (prefixes[i].data & TAIL_LENGTH_MASK)==maskedTail)
				{
				val=i;
				count++;
				}

		if(count==1)
			return val+1;
		else if(count==0)
			return -1;

		throw new RuntimeException("prefix Multismers: "+count+" From "+this.toString()+" to "+smerId+" with length "+(maskedTail>>TAIL_LENGTH_SHIFT));
	}

	// Which suffix of this smer links to the given smer-tailData combo

	private int suffixSmerIndex(long smerId, byte exists, long tailData)
	{
		int count=0,val=0;

		long maskedTail=tailData & TAIL_LENGTH_MASK;

		for(int i=0;i<suffixes.length;i++)
			{
			if(suffixes[i].smerExists==exists && suffixes[i].smerId==smerId && (suffixes[i].data & TAIL_LENGTH_MASK)==maskedTail)
				{
				val=i;
				count++;
				}
			}

		if(count==1)
			return val+1;
		else if(count==0)
			return -1;

		throw new RuntimeException("suffix Multismers: "+count+" From "+this.toString()+" to "+smerId+" with length "+(maskedTail>>TAIL_LENGTH_SHIFT));
	}


	public void verifyTailIndexes()
	{
		for(RouteEntry route: forwardRouteEntries)
			{
			if(route.prefix>prefixes.length)
				throw new RuntimeException("Dangling prefix in forward route - Found: "+route.prefix+" Max: "+prefixes.length);

			if(route.suffix>suffixes.length)
				throw new RuntimeException("Dangling suffix in forward route - Found: "+route.suffix+" Max: "+suffixes.length);
			}

		for(RouteEntry route: reverseRouteEntries)
			{
			if(route.prefix>prefixes.length)
				throw new RuntimeException("Dangling prefix in reverse route - Found: "+route.prefix+" Max: "+prefixes.length);

			if(route.suffix>suffixes.length)
				throw new RuntimeException("Dangling suffix in reverse route - Found: "+route.suffix+" Max: "+suffixes.length);
			}
	}

	public void verifyUniqueTails()
	{
		Set<Long> set=new HashSet<Long>();
		for(Tail prefix: prefixes)
			{
			if(set.contains(prefix.data))
				throw new RuntimeException("Non unique prefix: "+prefix.getSequence(WhichSequenceDirection.ORIGINAL)+" in "+this);

			set.add(prefix.data);
			}

		set.clear();

		for(Tail suffix: suffixes)
			{
			if(set.contains(suffix.data))
				throw new RuntimeException("Non unique suffix: "+suffix.getSequence(WhichSequenceDirection.ORIGINAL)+" in "+this);

			set.add(suffix.data);
			}
	}

	private int calcForwardWidthForPrefix(int targetPrefixIndex)
	{
		int width=0;
		for(RouteEntry route: forwardRouteEntries)
			{
			if(route.prefix==targetPrefixIndex)
				width+=route.width;
			}
		return width;
	}

	private int calcForwardWidthForSuffix(int targetSuffixIndex)
	{
		int width=0;
		for(RouteEntry route: forwardRouteEntries)
			{
			if(route.suffix==targetSuffixIndex)
				width+=route.width;
			}
		return width;
	}

	private int calcReverseWidthForPrefix(int targetPrefixIndex)
	{
		int width=0;
		for(RouteEntry route: reverseRouteEntries)
			{
			if(route.prefix==targetPrefixIndex)
				width+=route.width;
			}
		return width;
	}

	private int calcReverseWidthForSuffix(int targetSuffixIndex)
	{
		int width=0;
		for(RouteEntry route: reverseRouteEntries)
			{
			if(route.suffix==targetSuffixIndex)
				width+=route.width;
			}
		return width;
	}


	private static int findNodePositionForPrefix(RouteEntry routeTable[], int targetPrefixIndex, int edgePosition)
	{
		int nodePosition=0;

		for(RouteEntry route: routeTable)
			{
			if(route.prefix==targetPrefixIndex)
				{
				if(edgePosition<route.width)
					return nodePosition+edgePosition;
				else
					edgePosition-=route.width;
				}

			nodePosition+=route.width;
			}

		return -1;
	}

	private static int findNodePositionForSuffix(RouteEntry routeTable[], int targetSuffixIndex, int edgePosition)
	{
		int nodePosition=0;

		for(RouteEntry route: routeTable)
			{
			if(route.suffix==targetSuffixIndex)
				{
				if(edgePosition<route.width)
					return nodePosition+edgePosition;
				else
					edgePosition-=route.width;
				}

			nodePosition+=route.width;
			}

		return -1;
	}


	private RouteEntry findForwardRouteAndEdgeOffsetsForPrefix(int targetPrefixIndex, int targetPrefixOffset, int prefixOffsets[], int suffixOffsets[])
	{
		for(RouteEntry route: forwardRouteEntries)
			{
			if((route.prefix==targetPrefixIndex)&&(targetPrefixOffset>=prefixOffsets[route.prefix])&&(targetPrefixOffset<prefixOffsets[route.prefix]+route.width))
				return route;

			prefixOffsets[route.prefix]+=route.width;
			suffixOffsets[route.suffix]+=route.width;
			}

		return null;
	}

	private RouteEntry findForwardRouteAndEdgeOffsetsForSuffix(int targetSuffixIndex, int targetSuffixOffset, int prefixOffsets[], int suffixOffsets[])
	{
		for(RouteEntry route: forwardRouteEntries)
			{
			if((route.suffix==targetSuffixIndex)&&(targetSuffixOffset>=suffixOffsets[route.suffix])&&(targetSuffixOffset<suffixOffsets[route.suffix]+route.width))
				return route;

			prefixOffsets[route.prefix]+=route.width;
			suffixOffsets[route.suffix]+=route.width;
			}

		return null;
	}

	private RouteEntry findReverseRouteAndEdgeOffsetsForPrefix(int targetPrefixIndex, int targetPrefixOffset, int prefixOffsets[], int suffixOffsets[])
	{
		for(RouteEntry route: reverseRouteEntries)
			{
			if((route.prefix==targetPrefixIndex)&&(targetPrefixOffset>=prefixOffsets[route.prefix])&&(targetPrefixOffset<prefixOffsets[route.prefix]+route.width))
				return route;

			prefixOffsets[route.prefix]+=route.width;
			suffixOffsets[route.suffix]+=route.width;
			}

		return null;
	}

	private RouteEntry findReverseRouteAndEdgeOffsetsForSuffix(int targetSuffixIndex, int targetSuffixOffset, int prefixOffsets[], int suffixOffsets[])
	{
		for(RouteEntry route: reverseRouteEntries)
			{
			if((route.suffix==targetSuffixIndex)&&(targetSuffixOffset>=suffixOffsets[route.suffix])&&(targetSuffixOffset<suffixOffsets[route.suffix]+route.width))
				return route;

			prefixOffsets[route.prefix]+=route.width;
			suffixOffsets[route.suffix]+=route.width;
			}

		return null;
	}


	/*
	private int prefixOffsetOfRoute(int prefix, int routeIndex, boolean comp)
	{
		int prefixOffset=0;

		if(!comp)
			{
			for(int i=0;i<routeIndex;i++)
				if(forwardRoutes[i].prefix==prefix)
					prefixOffset+=forwardRoutes[i].width;
			}
		else
			{
			for(int i=revrsroutes.length-1;i>routeIndex;i--)
				if(routes[i].prefix==prefix)
					prefixOffset+=routes[i].width;
			}

		return prefixOffset;
	}

	private int suffixOffsetOfRoute(int suffix, int routeIndex, boolean comp) {
		int suffixOffset=0;

		if(!comp)
			{
			for(int i=0;i<routeIndex;i++)
				if(routes[i].suffix==suffix)
					suffixOffset+=routes[i].width;
			}
		else
			{
			for(int i=routes.length-1;i>routeIndex;i--)
				if(routes[i].suffix==suffix)
					suffixOffset+=routes[i].width;
			}

		return suffixOffset;
	}

	private void findPrefixRouteContext(int prefix, int prefixPosition, boolean comp, RouteContext context)
	{
		int prefixOffset=0;

		if(!comp)
			{
			for(int i=0;i<routes.length;i++)
				if(routes[i].prefix==prefix)
					{
					if(prefixOffset+routes[i].width>prefixPosition)
						{
						context.localPosition=prefixPosition-prefixOffset;
						context.routeIndex=i;
						return;
						}
					prefixOffset+=routes[i].width;
					}
			}
		else
			{
			for(int i=routes.length-1;i>=0;i--)
				if(routes[i].prefix==prefix)
					{
					if(prefixOffset+routes[i].width>prefixPosition)
						{
						context.localPosition=prefixPosition-prefixOffset;
						context.routeIndex=i;
						return;
						}

					prefixOffset+=routes[i].width;
					}
			}

		throw new RuntimeException("Failed to find prefixRouteContext");
	}

	private void findSuffixRouteContext(int suffix, int suffixPosition, boolean comp, RouteContext context)
	{
		int suffixOffset=0;

		if(!comp)
			{
			for(int i=0;i<routes.length;i++)
				if(routes[i].suffix==suffix)
					{
					if(suffixOffset+routes[i].width>suffixPosition)
						{
						context.localPosition=suffixPosition-suffixOffset;
						context.routeIndex=i;
						return;
						}
					suffixOffset+=routes[i].width;
					}
			}
		else
			{
			for(int i=routes.length-1;i>=0;i--)
				if(routes[i].suffix==suffix)
					{
					if(suffixOffset+routes[i].width>suffixPosition)
						{
						context.localPosition=suffixPosition-suffixOffset;
						context.routeIndex=i;
						return;
						}

					suffixOffset+=routes[i].width;
					}
			}

		throw new RuntimeException("Failed to find suffixRouteContext");
	}

	*/


	public static List<EdgeContext> makeSequenceEndTailContexts(LinkedSmer smer, boolean sequenceStart, boolean sequenceEnd, boolean forwardRoutes, boolean reverseRoutes)
	{
		List<EdgeContext> contexts=new ArrayList<EdgeContext>();
		if(forwardRoutes)
			{
			int prefixPositions[]=new int[smer.prefixes.length+1];
			int suffixPositions[]=new int[smer.suffixes.length+1];

			for(RouteEntry route: smer.forwardRouteEntries)
				{
				if(sequenceStart)
					{
					if(route.prefix==0 || smer.prefixes[route.prefix-1].smerExists==0) // Sequence starts at null / unlinked prefix
						{
						int tailPosition=prefixPositions[route.prefix];

						for(int i=0;i<route.width;i++)
							contexts.add(new EdgeContext(smer, route.prefix, WhichTail.PREFIX, WhichRouteTable.FORWARD, tailPosition+i));

						}
					}

				if(sequenceEnd)
					{
					if(route.suffix==0 || smer.suffixes[route.suffix-1].smerExists==0) // Sequence ends at null / unlinked suffix
						{
						int tailPosition=suffixPositions[route.suffix];

						for(int i=0;i<route.width;i++)
							contexts.add(new EdgeContext(smer, route.suffix, WhichTail.SUFFIX, WhichRouteTable.FORWARD, tailPosition+i));
						}
					}

				prefixPositions[route.prefix]+=route.width;
				suffixPositions[route.suffix]+=route.width;
				}
			}

		if(reverseRoutes)
			{
			int prefixPositions[]=new int[smer.prefixes.length+1];
			int suffixPositions[]=new int[smer.suffixes.length+1];

			for(RouteEntry route: smer.reverseRouteEntries)
				{
				if(sequenceStart)
					{
					if(route.suffix==0 || smer.suffixes[route.suffix-1].smerExists==0) // Sequence starts at null / unlinked suffix
						{
						int tailPosition=suffixPositions[route.suffix];

						for(int i=0;i<route.width;i++)
							contexts.add(new EdgeContext(smer, route.suffix, WhichTail.SUFFIX, WhichRouteTable.REVERSE, tailPosition+i));

						}
					}

				if(sequenceEnd)
					{
					if(route.prefix==0 || smer.prefixes[route.prefix-1].smerExists==0) // Sequence ends at null / unlinked prefix
						{
						int tailPosition=prefixPositions[route.prefix];

						for(int i=0;i<route.width;i++)
							contexts.add(new EdgeContext(smer, route.prefix, WhichTail.PREFIX, WhichRouteTable.REVERSE, tailPosition+i));
						}
					}

				prefixPositions[route.prefix]+=route.width;
				suffixPositions[route.suffix]+=route.width;
				}
			}


		return contexts;
	}



	public static List<EdgeContext> makeSequenceContexts(LinkedSmer smer, boolean forwardRoutes, boolean reverseRoutes)
	{
		List<EdgeContext> contexts=new ArrayList<EdgeContext>();
		if(forwardRoutes)
			{
			int prefixPositions[]=new int[smer.prefixes.length+1];
			int suffixPositions[]=new int[smer.suffixes.length+1];

			for(RouteEntry route: smer.forwardRouteEntries)
				{
				int tailPosition=prefixPositions[route.prefix];

				for(int i=0;i<route.width;i++)
					contexts.add(new EdgeContext(smer, route.prefix, WhichTail.PREFIX, WhichRouteTable.FORWARD, tailPosition+i));

				prefixPositions[route.prefix]+=route.width;
				suffixPositions[route.suffix]+=route.width;
				}
			}

		if(reverseRoutes)
			{
			int prefixPositions[]=new int[smer.prefixes.length+1];
			int suffixPositions[]=new int[smer.suffixes.length+1];

			for(RouteEntry route: smer.reverseRouteEntries)
				{
				int tailPosition=suffixPositions[route.suffix];

				for(int i=0;i<route.width;i++)
					contexts.add(new EdgeContext(smer, route.suffix, WhichTail.SUFFIX, WhichRouteTable.REVERSE, tailPosition+i));

				prefixPositions[route.prefix]+=route.width;
				suffixPositions[route.suffix]+=route.width;
				}
			}


		return contexts;
	}



	public static void makeSequenceSpanningContexts(LinkedSmer smer, boolean forwardRoutes, boolean reverseRoutes, List<EdgeContext> prefixContexts, List<EdgeContext> suffixContexts)
	{
		if(forwardRoutes && smer.forwardRouteEntries!=null)
			{
			int prefixPositions[]=new int[smer.prefixes.length+1];
			int suffixPositions[]=new int[smer.suffixes.length+1];

			for(RouteEntry route: smer.forwardRouteEntries)
				{
				int prefixPosition=prefixPositions[route.prefix];
				int suffixPosition=suffixPositions[route.suffix];

				for(int i=0;i<route.width;i++)
					{
					prefixContexts.add(new EdgeContext(smer, route.prefix, WhichTail.PREFIX, WhichRouteTable.FORWARD, prefixPosition+i));
					suffixContexts.add(new EdgeContext(smer, route.suffix, WhichTail.SUFFIX, WhichRouteTable.REVERSE, suffixPosition+i));
					}

				prefixPositions[route.prefix]+=route.width;
				suffixPositions[route.suffix]+=route.width;
				}
			}

		if(reverseRoutes && smer.reverseRouteEntries!=null)
			{
			int prefixPositions[]=new int[smer.prefixes.length+1];
			int suffixPositions[]=new int[smer.suffixes.length+1];

			for(RouteEntry route: smer.reverseRouteEntries)
				{
				int prefixPosition=prefixPositions[route.prefix];
				int suffixPosition=suffixPositions[route.suffix];

				for(int i=0;i<route.width;i++)
					{
					prefixContexts.add(new EdgeContext(smer, route.prefix, WhichTail.PREFIX, WhichRouteTable.FORWARD, prefixPosition+i));
					suffixContexts.add(new EdgeContext(smer, route.suffix, WhichTail.SUFFIX, WhichRouteTable.REVERSE, suffixPosition+i));
					}

				prefixPositions[route.prefix]+=route.width;
				suffixPositions[route.suffix]+=route.width;
				}
			}
	}





	public static class Tail
	{
		private long data;
		private long smerId;
		private byte smerExists;

		public Tail(long data, long smerId, byte smerExists) {
			this.data = data;
			this.smerId = smerId;
			this.smerExists = smerExists;
		}

		public long getData() {
			return data;
		}

		public long getSmerId() {
			return smerId;
		}

		public byte getSmerExists() {
			return smerExists;
		}

		public void extractSequence(StringBuilder sb, WhichSequenceDirection direction)
		{
			int length = (int)(data>>TAIL_LENGTH_SHIFT);
			long bases=data&TAIL_BASES_MASK;

			extractPackedSequence(bases, length, sb, direction);
		}

		public String getSequence(WhichSequenceDirection direction)
		{
			StringBuilder sb=new StringBuilder();
			extractSequence(sb,direction);
			return sb.toString();
		}


		@Override
		public String toString() {
			return "Tail [seq="
					+ getSequence(WhichSequenceDirection.ORIGINAL) + ", smerExists=" + smerExists
					+ ", smerId=" + smerId + "]";
		}

	}

	public static class RouteEntry
	{
		private short prefix;
		private short suffix;
		private int width;

		public RouteEntry(short prefix, short suffix, int width) {
			this.prefix = prefix;
			this.suffix = suffix;
			this.width = width;
		}

		public short getPrefix() {
			return prefix;
		}

		public short getSuffix() {
			return suffix;
		}

		public int getWidth() {
			return width;
		}

		public String toString() {
			return "RouteEntry [prefix=" + prefix +
					", suffix=" + suffix +
					", width="+width + "]";
		}

	}

	public static class RouteTag
	{
		private int nodePosition;
		private byte tagData[];

		public RouteTag(int nodePosition, byte[] tagData) {
			this.nodePosition = nodePosition;
			this.tagData = tagData;
		}

		public int getNodePosition() {
			return nodePosition;
		}

		public byte[] getTagData() {
			return tagData;
		}

		public String toString() {
			StringBuilder sb=new StringBuilder("RouteTag [nodePosition=");
			sb.append(nodePosition);
			sb.append(", tagData=");

			if(tagData==null)
				sb.append("null");
			else if(tagData.length==0)
				sb.append("[]");
			else
				{
				int val=((int)tagData[0]) & 0xFF;
				sb.append("[");
				sb.append(val);

				for(int i=1;i<tagData.length;i++)
					{
					val=((int)tagData[i]) & 0xFF;
					sb.append(",");
					sb.append(val);
					}
				sb.append("]");
				}

			sb.append("]");

			return sb.toString();
		}


	}

	public static class NodeContext
	{
		private LinkedSmer smer;
		private WhichRouteTable routeTable;
		private int nodePosition;

		public NodeContext(LinkedSmer smer, WhichRouteTable routeTable, int nodePosition)
		{
			this.smer=smer;
			this.routeTable=routeTable;
			this.nodePosition=nodePosition;
		}

		public EdgeContext transitionToEdge(WhichSequenceDirection direction)
		{
			return null;
		}

	}

	public static class EdgeContext
	{
		private LinkedSmer smer;
		private int tailIndex;
		private WhichTail whichTail;
		private WhichRouteTable routeTable;
		private int edgePosition;

		public EdgeContext(LinkedSmer smer, int tailIndex, WhichTail whichTail, WhichRouteTable routeTable, int edgePosition)
		{
			this.smer=smer;
			this.tailIndex=tailIndex;
			this.whichTail=whichTail;
			this.routeTable=routeTable;
			this.edgePosition=edgePosition;
		}

		public NodeContext transitionToNode(WhichSequenceDirection direction)
		{
			return null;
		}

		public EdgeContext transitionAcrossNode(WhichSequenceDirection direction)
		{
			int prefixOffsets[]=new int[smer.prefixes.length+1];
			int suffixOffsets[]=new int[smer.suffixes.length+1];

			if(whichTail==WhichTail.PREFIX) // Level 1: Transitioning from a prefix
				{
				if(routeTable==WhichRouteTable.FORWARD) // Level 2: Transitioning from a prefix of a canonical node, implies forward direction
					{
					if(direction!=WhichSequenceDirection.ORIGINAL)
						throw new RuntimeException("AcrossNode transition from Prefix of canonical node requires forward direction");

					RouteEntry targetRoute=smer.findForwardRouteAndEdgeOffsetsForPrefix(tailIndex, edgePosition, prefixOffsets, suffixOffsets);
					if(targetRoute==null)
						throw new RuntimeException("Unable to find route for "+whichTail+" with index "+tailIndex+" and position "+edgePosition+" in "+smer+" with max "+prefixOffsets[tailIndex]);

					int routeOffset=edgePosition-prefixOffsets[tailIndex];

					return new EdgeContext(smer, targetRoute.suffix, WhichTail.SUFFIX, WhichRouteTable.FORWARD, suffixOffsets[targetRoute.suffix]+routeOffset);
					}
				else
					{
					if(direction!=WhichSequenceDirection.REVERSECOMP)
						throw new RuntimeException("AcrossNode transition from Prefix of complementary node requires reverse direction");

					RouteEntry targetRoute=smer.findReverseRouteAndEdgeOffsetsForPrefix(tailIndex, edgePosition, prefixOffsets, suffixOffsets);
					if(targetRoute==null)
						throw new RuntimeException("Unable to find route for "+whichTail+" with index "+tailIndex+" and position "+edgePosition+" in "+smer+" with max "+prefixOffsets[tailIndex]);

					int routeOffset=edgePosition-prefixOffsets[tailIndex];

					return new EdgeContext(smer, targetRoute.suffix, WhichTail.SUFFIX, WhichRouteTable.REVERSE, suffixOffsets[targetRoute.suffix]+routeOffset);
					}
				}
			else // Level 1: Transitioning from a suffix
				{
				if(routeTable==WhichRouteTable.FORWARD) // Level 2: Transitioning from a prefix of a canonical node, implies reverse direction
					{
					if(direction!=WhichSequenceDirection.REVERSECOMP)
						throw new RuntimeException("AcrossNode transition from Suffix of complement node requires reverse direction");

					RouteEntry targetRoute=smer.findForwardRouteAndEdgeOffsetsForSuffix(tailIndex, edgePosition, prefixOffsets, suffixOffsets);
					if(targetRoute==null)
						throw new RuntimeException("Unable to find route for "+whichTail+" with index "+tailIndex+" and position "+edgePosition+" in "+smer+" with max "+suffixOffsets[tailIndex]);

					int routeOffset=edgePosition-suffixOffsets[tailIndex];

					return new EdgeContext(smer, targetRoute.prefix, WhichTail.PREFIX, WhichRouteTable.FORWARD, prefixOffsets[targetRoute.prefix]+routeOffset);
					}
				else
					{
					if(direction!=WhichSequenceDirection.ORIGINAL)
						throw new RuntimeException("AcrossNode transition from Prefix of complement node requires forward direction");

					RouteEntry targetRoute=smer.findReverseRouteAndEdgeOffsetsForSuffix(tailIndex, edgePosition, prefixOffsets, suffixOffsets);
					if(targetRoute==null)
						throw new RuntimeException("Unable to find route for "+whichTail+" with index "+tailIndex+" and position "+edgePosition+" in "+smer+" with max "+suffixOffsets[tailIndex]);

					int routeOffset=edgePosition-suffixOffsets[tailIndex];

					return new EdgeContext(smer, targetRoute.prefix, WhichTail.PREFIX, WhichRouteTable.REVERSE, prefixOffsets[targetRoute.prefix]+routeOffset);
					}
				}
		}

		public RouteTag getTagData()
		{
			RouteEntry rtEntries[]=routeTable==WhichRouteTable.FORWARD?smer.forwardRouteEntries:smer.reverseRouteEntries;

			int nodePosition=whichTail==WhichTail.PREFIX?
					LinkedSmer.findNodePositionForPrefix(rtEntries, tailIndex, edgePosition):LinkedSmer.findNodePositionForSuffix(rtEntries, tailIndex, edgePosition);

			RouteTag tags[]=routeTable==WhichRouteTable.FORWARD?smer.forwardRouteTags:smer.reverseRouteTags;

			for(RouteTag tag: tags)
				if(tag.nodePosition == nodePosition)
					return tag;

			return null;
		}


		public static void verifyTailWidth(LinkedSmer smer1, WhichTail which1, int index1, LinkedSmer smer2, WhichTail which2, int index2, byte exists)
		{
			int width1F, width1R, width2F, width2R;

			if(which1==WhichTail.PREFIX)
				{
				width1F=smer1.calcForwardWidthForPrefix(index1);
				width1R=smer1.calcReverseWidthForPrefix(index1);
				}
			else
				{
				width1F=smer1.calcForwardWidthForSuffix(index1);
				width1R=smer1.calcReverseWidthForSuffix(index1);
				}

			if(which2==WhichTail.PREFIX)
				{
				width2F=smer2.calcForwardWidthForPrefix(index2);
				width2R=smer2.calcReverseWidthForPrefix(index2);
				}
			else
				{
				width2F=smer2.calcForwardWidthForSuffix(index2);
				width2R=smer2.calcReverseWidthForSuffix(index2);
				}

			if(exists<0)
				{
				if(width1F!=width2R || width1R!=width2F)
					{
					throw new RuntimeException("Linking edge width mismatch (ori: "+exists+") "+width1F+" "+width1R+" "+width2F+" "+width2R+
						"\n"+which1+" "+index1+
						"\n"+smer1+
						"\n"+which2+" "+index2+
						"\n"+smer2);
					}
				}
			else
				{
				if(width1F!=width2F || width1R!=width2R)
					{
					throw new RuntimeException("Linking edge width mismatch (ori: "+exists+") "+width1F+" "+width1R+" "+width2F+" "+width2R+
						"\n"+which1+" "+index1+
						"\n"+smer1+
						"\n"+which2+" "+index2+
						"\n"+smer2);
					}
				}
		}


		public EdgeContext transitionToLinkingEdge(WhichSequenceDirection direction, LinkedSmer targetSmer)
		{
			if(tailIndex==0)
				return null;

			if(whichTail==WhichTail.PREFIX) // Level 1: Linking from a prefix
				{
				Tail tail=smer.prefixes[tailIndex-1];

				if(tail.smerExists==0)
					return null;

				if(targetSmer==null)
					throw new RuntimeException("Null target smer");

				if(targetSmer.getSmerId()!=tail.smerId)
					throw new RuntimeException("Incorrect target smer. Got: "+targetSmer.getSmerId()+" Expected: "+tail.smerId);

				if(routeTable==WhichRouteTable.FORWARD) // Level 2: Linking from a prefix of a canonical Node, implies reverse direction
					{
					if(direction!=WhichSequenceDirection.REVERSECOMP)
						throw new RuntimeException("LinkingEdgeTransition from prefix of canonical node requires reverse direction");

					if(tail.smerExists>0) // Reverse to target in same orientation (canonical): Arriving at suffix, forward routing table
						{
						int targetTailIndex=targetSmer.suffixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do suffix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking suffix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.SUFFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 0 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.SUFFIX, WhichRouteTable.FORWARD, edgePosition);
						}
					else // Reverse to target in same orientation (complement): Arriving at prefix, reverse routing table
						{
						int targetTailIndex=targetSmer.prefixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do prefix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking prefix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.PREFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 1 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.PREFIX, WhichRouteTable.REVERSE, edgePosition);
						}

					}
				else // Level 2: Linking from prefix of a Complementary Node, implies forward direction
					{
					if(direction!=WhichSequenceDirection.ORIGINAL)
						throw new RuntimeException("LinkingEdgeTransition from prefix of complmentary node requires forward direction");

					if(tail.smerExists>0) // Forward to Target in same orientation (complement): Arriving at suffix, reverse routing table
						{
						int targetTailIndex=targetSmer.suffixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do suffix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking suffix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.SUFFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 2 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.SUFFIX, WhichRouteTable.REVERSE, edgePosition);
						}
					else // Forward to Target in opposite orientation (canonical): Arriving at prefix, forward routing table
						{
						int targetTailIndex=targetSmer.prefixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do prefix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking prefix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.PREFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 3 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.PREFIX, WhichRouteTable.FORWARD, edgePosition);
						}
					}
				}
			else // Level 1: Linking from a suffix
				{
				Tail tail=smer.suffixes[tailIndex-1];

				if(tail.smerExists==0)
					return null;

				if(targetSmer==null)
					throw new RuntimeException("Null target smer");

				if(targetSmer.getSmerId()!=tail.smerId)
					throw new RuntimeException("Incorrect target smer. Got: "+targetSmer.getSmerId()+" Expected: "+tail.smerId);

				if(routeTable==WhichRouteTable.FORWARD) // Level 2: Linking from a suffix of a canonical Node, implies forward direction
					{
					if(direction!=WhichSequenceDirection.ORIGINAL)
						throw new RuntimeException("LinkingEdgeTransition from suffix of conanonical node requires forward direction");

					if(tail.smerExists>0) // Forward to target in same orientation (canonical): Arriving at prefix, forward routing table
						{
						int targetTailIndex=targetSmer.prefixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do prefix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking prefix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.PREFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 4 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.PREFIX, WhichRouteTable.FORWARD, edgePosition);
						}
					else // Forward to target in opposite orientation (complement): Arriving at suffix, reverse routing table
						{
						int targetTailIndex=targetSmer.suffixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do suffix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking suffix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.SUFFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 5 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.SUFFIX, WhichRouteTable.REVERSE, edgePosition);
						}
					}
				else  // Level 2: Linking from a suffix of a complement Node, implies reverse direction
					{
					if(direction!=WhichSequenceDirection.REVERSECOMP)
						throw new RuntimeException("LinkingEdgeTransition from suffix of complmentary node requires reverse direction");

					if(tail.smerExists>0) // Reverse to target in same orientation (complement): Arriving at prefix, reverse routing table
						{
						int targetTailIndex=targetSmer.prefixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do prefix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking prefix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.PREFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 6 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.PREFIX, WhichRouteTable.REVERSE, edgePosition);
						}
					else // Reverse to target in opposite orientation (canonical): Arriving at suffix, forward routing table
						{
						int targetTailIndex=targetSmer.suffixSmerIndex(smer.getSmerId(), tail.smerExists, tail.data); // do suffix lookup
						if(targetTailIndex<0)
							throw new RuntimeException("Unable to find linking suffix for "+tail+" in "+tail.smerExists+" of "+targetSmer+" to "+smer);

						verifyTailWidth(smer,whichTail,tailIndex,targetSmer,WhichTail.SUFFIX,targetTailIndex,tail.smerExists);

						//System.out.println("Transition 7 - "+targetTailIndex);
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.SUFFIX, WhichRouteTable.FORWARD, edgePosition);
						}
					}

				}
		}


		public long getLinkingSmerId()
		{
			if(tailIndex==0)
				return -1;

			if(whichTail==WhichTail.PREFIX)
				{
				if(smer.prefixes[tailIndex-1].smerExists==0)
					return -1;

				return smer.prefixes[tailIndex-1].smerId;
				}
			else
				{
				if(smer.suffixes[tailIndex-1].smerExists==0)
					return -1;

				return smer.suffixes[tailIndex-1].smerId;
				}

		}

		public void extractSequence(StringBuilder sb, WhichSequenceDirection direction, boolean includeNode)
		{
//			smer, tailIndex, whichTail, routeTable, edgePosition

			// whichTail PREFIX vs SUFFIX
			// whichRouteTable FORWARD vs REVERSE

			//Prefix forward = !direction
			//Suffix forward = direction

			//Prefix reverse = direction
			//Suffix reverse = !direction

			if(routeTable==WhichRouteTable.REVERSE)
				direction=WhichSequenceDirection.other(direction);

			Tail tail;
			WhichSequenceDirection tailDirection;

			if(tailIndex>0)
				{
				if(whichTail==WhichTail.PREFIX)
					{
					tail=smer.prefixes[tailIndex-1];
					tailDirection=WhichSequenceDirection.other(direction);
					}
				else
					{
					tail=smer.suffixes[tailIndex-1];
					tailDirection=direction;
					}

				tail.extractSequence(sb, tailDirection);
				}


			if(includeNode)
				smer.extractSequence(sb, direction);

		}



		/*
		public static TailContext makeTailContext(LinkedSmer smer, WhichRouteTable routeTable, int routeIndex, WhichSequenceDirection seqDirection)
		{
			return null;
		}
*/



		/*
		public static Context makeReadBeginContext(LinkedSmer smer, WhichTail tail, int routeIndex, int localPosition)
		{
			Route route=smer.routes[routeIndex];

			if(tail==WhichTail.PREFIX)
				return new Context(smer, false, route.prefix, WhichTail.PREFIX, smer.prefixOffsetOfRoute(route.prefix, routeIndex, false)+localPosition);
			else
				return new Context(smer, true, route.suffix, WhichTail.SUFFIX, smer.suffixOffsetOfRoute(route.suffix, routeIndex, true)+localPosition);
		}
				private LinkedSmer smer;
		private int tailIndex;
		private WhichTail tail;
		private WhichRouteTable routeTable;
		private int tailPosition;
		private WhichSequenceDirection seqDirection;
	*/
		public LinkedSmer getSmer() {
			return smer;
		}

		public int getTailIndex() {
			return tailIndex;
		}

		public WhichTail whichTail() {
			return whichTail;
		}

		public WhichRouteTable whichRouteTable()
		{
			return routeTable;
		}

		public int getEdgePosition()
		{
			return edgePosition;
		}

		@Override
		public String toString() {
			return "Context [smer=" + smer.smerId + ", tailIndex=" + tailIndex + ", whichTail=" + whichTail
					+ ", routeTable=" + routeTable + ", tailPosition=" + edgePosition
					+ "]";
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			EdgeContext other = (EdgeContext) obj;
			if (smer.smerId != other.smer.smerId)
				return false;
			if (tailIndex != other.tailIndex)
				return false;
			if (!whichTail.equals(other.whichTail))
				return false;
			if (routeTable != other.routeTable)
				return false;
			if (edgePosition != other.edgePosition)
				return false;

			return true;
		}

		/*
		private void appendTail(StringBuilder sb)
		{
			if(tailIndex>0)
				{
				if(tail==WhichTail.PREFIX)
					{
					String seq=smer.prefixes[tailIndex-1].getTailSequence(true);
					if(compFlag)
						sb.append(complementSequence(seq));
					else
						sb.append(seq);
					}
				else
					{
					String seq=smer.suffixes[tailIndex-1].getTailSequence(false);
					if(compFlag)
						sb.append(complementSequence(seq));
					else
						sb.append(seq);
					}
				}

		}
		*/
/*
		public String readToEnd(LinkedSmerCache cache, int s, int maxSteps)
		{
			StringBuilder sb=new StringBuilder();

			Context currentCon=this,prevCon,backCon;

			// Add starting tail and smer

			appendTail(sb);
			String smerSeq=smer.unpackSmer(s);

			if(compFlag)
				sb.append(complementSequence(smerSeq));
			else
				sb.append(smerSeq);

			while(currentCon!=null && maxSteps>0)
				{
				prevCon=currentCon;
				currentCon=currentCon.swapTail();
				backCon=currentCon.swapTail();

				if(!prevCon.equals(backCon))
					throw new RuntimeException("SwapTail back fail: \n"+prevCon.toString()+"\n"+currentCon.toString()+"\n"+backCon.toString());

				currentCon.appendTail(sb);

				prevCon=currentCon;
				currentCon=currentCon.followTailLink(cache);

				if(currentCon!=null)
					{
					backCon=currentCon.followTailLink(cache);

					if(!prevCon.equals(backCon))
						throw new RuntimeException("FollowTailLink back fail: \n"+prevCon.toString()+"\n"+currentCon.toString()+"\n"+backCon.toString());
					}

				maxSteps--;
				}

			if(maxSteps>0)
				return sb.toString();
			else
				{
				throw new RuntimeException("Failed to read sequence end");
				}
		}

		*/


		/*


		public Context swapTail()
		{
			Route routes[]=smer.getRoutes();
			RouteContext rc=new RouteContext();

			if(tail==WhichTail.PREFIX)
				{
				if(!compFlag) // Prefix -> Suffix: Normal orientation
					{
					smer.findPrefixRouteContext(tailIndex, tailPosition, false, rc);
					int suffix=routes[rc.routeIndex].suffix;
					int suffixOffset=smer.suffixOffsetOfRoute(suffix,rc.routeIndex,false);

					return new Context(smer, false, suffix, WhichTail.SUFFIX, suffixOffset+rc.localPosition);
					}
				else		  // Prefix -> Suffix: Complement orientation
					{
					smer.findPrefixRouteContext(tailIndex, tailPosition, true, rc);
					int suffix=routes[rc.routeIndex].suffix;
					int suffixOffset=smer.suffixOffsetOfRoute(suffix,rc.routeIndex,true);

					return new Context(smer, true, suffix, WhichTail.SUFFIX, suffixOffset+rc.localPosition);
					}
				}
			else
				{
				if(!compFlag) // Suffix -> Prefix: Normal orientation
					{
					smer.findSuffixRouteContext(tailIndex, tailPosition, false, rc);
					int prefix=routes[rc.routeIndex].prefix;
					int prefixOffset=smer.prefixOffsetOfRoute(prefix,rc.routeIndex,false);

					return new Context(smer, false, prefix, WhichTail.PREFIX, prefixOffset+rc.localPosition);
					}
				else		  // Suffix -> Prefix: Complement orientation
					{
					smer.findSuffixRouteContext(tailIndex, tailPosition, true, rc);
					int prefix=routes[rc.routeIndex].prefix;
					int prefixOffset=smer.prefixOffsetOfRoute(prefix,rc.routeIndex,true);

					return new Context(smer, true, prefix, WhichTail.PREFIX, prefixOffset+rc.localPosition);
					}
				}
		}

		public Context followTailLink(LinkedSmerCache cache)
		{

			if(tailIndex==0)
				return null;

			int corTailIndex=tailIndex-1;

			if(tail==WhichTail.PREFIX)
				{
				if(smer.prefixes[corTailIndex].smerExists)
					{
					LinkedSmer newSmer=cache.getLinkedSmer(smer.prefixes[corTailIndex].smerId);

					if(!smer.prefixes[corTailIndex].compFlag)
						{
						//System.out.println("FT1");

						int newPrefix=0;//newSmer.prefixSmerIndex(smer.smerId)+1;
						int newSuffix=newSmer.suffixSmerIndex(smer.smerId, false, smer.prefixes[corTailIndex].data)+1;

//						int newSuffixCor=newSuffix-1;

//						if((newSuffix==0)||(!newSmer.suffixes[newSuffixCor].smerExists)||(newSmer.suffixes[newSuffixCore].)

						if(newSuffix==0)
							throw new RuntimeException("pre: "+newPrefix+" SUF: "+newSuffix);

						return new Context(newSmer,compFlag,newSuffix,WhichTail.SUFFIX,tailPosition);
						}
					else
						{
						//System.out.println("FT2");

						int newPrefix=newSmer.prefixSmerIndex(smer.smerId, true, smer.prefixes[corTailIndex].data)+1;
						int newSuffix=0;//newSmer.suffixSmerIndex(smer.smerId)+1;

						if(newPrefix==0)
							throw new RuntimeException("PRE: "+newPrefix+" suf: "+newSuffix);

						return new Context(newSmer,!compFlag,newPrefix,WhichTail.PREFIX,tailPosition);
						}
					}
				else
					return null;
				}
			else
				{
				if(smer.suffixes[corTailIndex].smerExists)
					{
					LinkedSmer newSmer=cache.getLinkedSmer(smer.suffixes[corTailIndex].smerId);

					if(!smer.suffixes[corTailIndex].compFlag)
						{
						//System.out.println("FT3");

						int newPrefix=newSmer.prefixSmerIndex(smer.smerId, false, smer.suffixes[corTailIndex].data)+1;
						int newSuffix=0;//newSmer.suffixSmerIndex(smer.smerId)+1;

						if(newPrefix==0)
							throw new RuntimeException("PRE: "+newPrefix+" suf: "+newSuffix);

						return new Context(newSmer,compFlag,newPrefix,WhichTail.PREFIX,tailPosition);
						}
					else
						{
						//System.out.println("FT4");

						int newPrefix=0;//newSmer.prefixSmerIndex(smer.smerId, smer.suffixes[corTailIndex].data)+1;
						int newSuffix=newSmer.suffixSmerIndex(smer.smerId, true, smer.suffixes[corTailIndex].data)+1;

						if(newSuffix==0)
							throw new RuntimeException("pre: "+newPrefix+" SUF: "+newSuffix);

						return new Context(newSmer,!compFlag,newSuffix,WhichTail.SUFFIX,tailPosition);
						}
					}
				else
					return null;
				}
			}
		*/
	}


}
