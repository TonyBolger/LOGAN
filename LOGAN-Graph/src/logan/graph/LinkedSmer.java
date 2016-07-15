package logan.graph;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

public class LinkedSmer {


	public static enum WhichTail
	{
		PREFIX(),
		SUFFIX();

		static WhichTail other(WhichTail tail)
		{
			return tail==PREFIX?SUFFIX:PREFIX;
		}
	}

	public static enum WhichRouteTable
	{
		FORWARD(),
		REVERSE();
	}

	public static enum WhichSequenceDirection
	{
		ORIGINAL(),
		REVERSECOMP();
	}


	private long smerId;
	private Tail prefixes[];
	private Tail suffixes[];
	private Route forwardRoutes[];
	private Route reverseRoutes[];

	public LinkedSmer(long smerId, Tail prefixes[], Tail suffixes[], Route forwardRoutes[], Route reverseRoutes[])
	{
		this.smerId=smerId;
		this.prefixes=prefixes;
		this.suffixes=suffixes;
		this.forwardRoutes=forwardRoutes;
		this.reverseRoutes=reverseRoutes;
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

	public Route[] getForwardRoutes() {
		return forwardRoutes;
	}

	public Route[] getReverseRoutes() {
		return reverseRoutes;
	}

	public String toString()
	{
		StringBuilder sb=new StringBuilder();
		sb.append("LinkedSmer [ID: "+smerId);
		sb.append(", Seq: "+unpackSmer(23));
		sb.append(", Prefixes: [");
		for(Tail p: prefixes)
			sb.append(p.toString(true));
		sb.append("], Suffixes: [");
		for(Tail s: suffixes)
			sb.append(s.toString(false));
		sb.append("], ForwardRoutes: [");
		for(Route r: forwardRoutes)
			sb.append(r.toString());
		sb.append("], ReverseRoutes: [");
		for(Route r: reverseRoutes)
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


	private String unpackSmer(int s)
	{
		StringBuilder sb = new StringBuilder();

		for (int i = s-1; i >= 0; i--)
			{
			int base = (int)(smerId >> 2 * i);

			switch (base & 0x3)
				{
				case 0:
					sb.append("A");
					break;

				case 1:
					sb.append("C");
					break;

				case 2:
					sb.append("G");
					break;

				case 3:
					sb.append("T");
					break;
				}
			}

		return sb.toString();
	}


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



	// Which prefix of this smer links to the given smer-tailData combo

	// Prefix     <- this
	// SmerId -> tailData

	private static long TAIL_LENGTH_MASK=0xFFFF000000000000L;

	private int prefixSmerIndex(long smerId, boolean compFlag, long tailData)
	{
		int count=0,val=0;

		long maskedTail=tailData & TAIL_LENGTH_MASK;
		byte exists=(byte)(compFlag?-1:1);

		for(int i=0;i<prefixes.length;i++)
			if(prefixes[i].smerExists==exists && prefixes[i].smerId==smerId && (prefixes[i].data & TAIL_LENGTH_MASK)==maskedTail)
				{
				val=i;
				count++;
			}

		if(count==1)
			return val;
		else if(count==0)
			return -1;

		throw new RuntimeException("prefix Multismers: "+count+" From "+this.toString()+" to "+smerId+" with length "+(maskedTail>>48));
	}

	// Which suffix of this smer links to the given smer-tailData combo

	private int suffixSmerIndex(long smerId, boolean compFlag, long tailData)
	{
		int count=0,val=0;

		long maskedTail=tailData & TAIL_LENGTH_MASK;
		byte exists=(byte)(compFlag?-1:1);

		for(int i=0;i<suffixes.length;i++)
			{
			if(suffixes[i].smerExists==exists && suffixes[i].smerId==smerId && (suffixes[i].data & TAIL_LENGTH_MASK)==maskedTail)
				{
				val=i;
				count++;
				}
			}

		if(count==1)
			return val;
		else if(count==0)
			return -1;

		throw new RuntimeException("suffix Multismers: "+count+" From "+this.toString()+" to "+smerId+" with length "+(maskedTail>>48));
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


	public static Collection<EdgeContext> makeSequenceEndTailContexts(LinkedSmer smer, boolean sequenceStart, boolean sequenceEnd, boolean forwardRoutes, boolean reverseRoutes)
	{
		List<EdgeContext> contexts=new ArrayList<EdgeContext>();
		if(forwardRoutes)
			{
			int prefixPositions[]=new int[smer.prefixes.length+1];
			int suffixPositions[]=new int[smer.suffixes.length+1];

			for(Route route: smer.forwardRoutes)
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

			for(Route route: smer.reverseRoutes)
				{
				if(sequenceStart)
					{
					if(route.suffix==0 || smer.suffixes[route.suffix-1].smerExists==0) // Sequence starts at null / unlinked suffix
						{
						int tailPosition=prefixPositions[route.suffix];

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

		public byte isSmerExists() {
			return smerExists;
		}

		final static char canonicalBases[]={'A','C','G','T'};
		final static char complementBases[]={'T','G','C','A'};

		public String getTailSequence(boolean reverseComplement)
		{
			int len = (int)(data>>48);

			StringBuilder sb = new StringBuilder();

			long base = data;

			char decode[]=reverseComplement?complementBases:canonicalBases;

			for (int i = 0; i < len; i++)
				{
				sb.append(decode[(int)(base & 0x3)]);
				base>>=2;
				}

			if(reverseComplement)
				return sb.toString();
			else
				return sb.reverse().toString();
		}


		@Override
		public String toString() {
			return "Tail [seq="
					+ getTailSequence(false) + ", smerExists=" + smerExists
					+ ", smerId=" + smerId + "]";
		}

		public String toString(boolean rev) {
			return "Tail [seq="
					+ getTailSequence(rev) + ", smerExists=" + smerExists
					+ ", smerId=" + smerId + "]";
		}

	}

	public static class Route
	{
		private short prefix;
		private short suffix;
		private int width;

		public Route(short prefix, short suffix, int width) {
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
			return "Route [prefix=" + prefix +
					", suffix=" + suffix +
					", width="+width + "]";
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
		private WhichTail tail;
		private WhichRouteTable routeTable;
		private int edgePosition;

		public EdgeContext(LinkedSmer smer, int tailIndex, WhichTail tail, WhichRouteTable routeTable, int edgePosition)
		{
			this.smer=smer;
			this.tailIndex=tailIndex;
			this.tail=tail;
			this.routeTable=routeTable;
			this.edgePosition=edgePosition;
		}

		public NodeContext transitionToNode(WhichSequenceDirection direction)
		{
			return null;
		}

		public EdgeContext transitionAcrossNode(WhichSequenceDirection direction)
		{
			return null;
		}

		public EdgeContext transitionToLinkingEdge(WhichSequenceDirection direction, LinkedSmer targetSmer)
		{
			if(tailIndex==0)
				return null;

			if(targetSmer==null)
				throw new RuntimeException("Null target smer");

			if(tail==WhichTail.PREFIX) // Level 1: Linking from a prefix
				{
				byte exists=smer.prefixes[tailIndex-1].smerExists;

				if(exists==0)
					return null;

				long targetSmerId=smer.prefixes[tailIndex-1].smerId;

				if(targetSmer.getSmerId()!=targetSmerId)
					throw new RuntimeException("Incorrect target smer. Got: "+targetSmer.getSmerId()+" Expected: "+targetSmerId);

				if(routeTable==WhichRouteTable.FORWARD) // Level 2: Linking from a prefix of a canonical Node, implies reverse direction
					{
					if(direction!=WhichSequenceDirection.REVERSECOMP)
						throw new RuntimeException("LinkingEdgeTransition from prefix of canonical node requires reverse direction");

					if(exists>0) // Reverse to Canonical Target: Arriving at suffix, forward routing table
						{
						int targetTailIndex=0; // do suffix lookup
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.SUFFIX, WhichRouteTable.FORWARD, edgePosition);
						}
					else // Reverse to Complement Target: Arriving at prefix, reverse routing table
						{
						int targetTailIndex=0; // do prefix lookup
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.PREFIX, WhichRouteTable.REVERSE, edgePosition);
						}

					}
				else // Level 2: Linking from prefix of a Complementary Node, implies forward direction
					{
					if(direction!=WhichSequenceDirection.ORIGINAL)
						throw new RuntimeException("LinkingEdgeTransition from prefix of complmentary node requires forward direction");

					if(exists>0) // Forward to Canonical Target: Arriving at prefix, forward routing table
						{
						int targetTailIndex=0; // do prefix lookup
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.PREFIX, WhichRouteTable.FORWARD, edgePosition);
						}
					else // Forward to Complement target: Arriving at suffix, reverse routing table
						{
						int targetTailIndex=0; // do suffix lookup
						return new EdgeContext(targetSmer, targetTailIndex, WhichTail.SUFFIX, WhichRouteTable.REVERSE, edgePosition);
						}
					}
				}
			else // Level 1: Linking from a suffix
				{
				byte exists=smer.suffixes[tailIndex-1].smerExists;

				if(exists==0)
					return null;

				long targetSmerId=smer.suffixes[tailIndex-1].smerId;

				if(targetSmer.getSmerId()!=targetSmerId)
					throw new RuntimeException("Incorrect target smer. Got: "+targetSmer.getSmerId()+" Expected: "+targetSmerId);

				if(routeTable==WhichRouteTable.FORWARD) // Level 2: Linking from a suffix of a canonical Node, implies forward direction
					{
					if(direction!=WhichSequenceDirection.ORIGINAL)
						throw new RuntimeException("LinkingEdgeTransition from suffix of conanonical node requires forward direction");

					if(exists>0) // Forward to Canonical Target: Arriving at prefix, forward routing table
						{

						}
					else // Forward to Complement Target: Arriving at suffix, reverse routing table
						{

						}

					}
				else  // Level 2: Linking from a suffix of a complement Node, implies reverse direction
					{
					if(direction!=WhichSequenceDirection.REVERSECOMP)
						throw new RuntimeException("LinkingEdgeTransition from suffix of complmentary node requires reverse direction");

					if(exists>0) // Reverse to Canonical Target: Arriving at suffix, forward routing table
						{

						}
					else // Reverse to Complement Target: Arriving at prefix, reverse routing table
						{

						}
					}

				}



			return null;
		}


		public long getLinkingSmerId()
		{
			if(tailIndex==0)
				return -1;

			if(tail==WhichTail.PREFIX)
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
			return tail;
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
			return "Context [smer=" + smer.smerId + ", tailIndex=" + tailIndex + ", tail=" + tail
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
			if (!tail.equals(other.tail))
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
