package logan.graph;



public class LinkedSmer {
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
			sb.append(p.toString());
		sb.append("], Suffixes: [");
		for(Tail s: suffixes)
			sb.append(s.toString());
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
	
	/*
	private int prefixSmerIndex(long smerId, boolean compFlag, byte tailData[])
	{
		int count=0,val=0;
		
		for(int i=0;i<prefixes.length;i++)
			if(prefixes[i].smerExists && prefixes[i].smerId==smerId && prefixes[i].compFlag==compFlag && prefixes[i].data[0]==tailData[0])
				{
				val=i;
				count++;
				}
		
		if(count==1)
			return val;
		else if(count==0)
			return -1;

		throw new RuntimeException("prefix Multismers: "+count+" From "+this.toString()+" to "+smerId+" with length "+tailData[0]);
	}

	// Which suffix of this smer links to the given smer-tailData combo
	
	private int suffixSmerIndex(long smerId, boolean compFlag, byte tailData[])
	{
		int count=0,val=0;
		
		for(int i=0;i<suffixes.length;i++)
			{
			if(suffixes[i].smerExists && suffixes[i].smerId==smerId && suffixes[i].compFlag==compFlag && suffixes[i].data[0]==tailData[0])
				{
				val=i;
				count++;
				}
			}
		
		if(count==1)
			return val;
		else if(count==0)
			return -1;

		throw new RuntimeException("suffix Multismers: "+count+" From "+this.toString()+" to "+smerId+" with length "+tailData[0]);
	}
	*/
	
	/*
	private int prefixOffsetOfRoute(int prefix, int routeIndex, boolean comp) {
		int prefixOffset=0;
		
		if(!comp)
			{
			for(int i=0;i<routeIndex;i++)
				if(routes[i].prefix==prefix)
					prefixOffset+=routes[i].width;
			}
		else 
			{
			for(int i=routes.length-1;i>routeIndex;i--)
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
	
	public static class Tail
	{
		private byte data[];
		private long smerId;
		private boolean smerExists;
		
		public Tail(byte data[], long smerId, boolean smerExists) {
			this.data = data;
			this.smerId = smerId;
			this.smerExists = smerExists;
		}

		public byte[] getData() {
			return data;
		}

		public long getSmerId() {
			return smerId;
		}

		public boolean isSmerExists() {
			return smerExists;
		}

		public String getTailSequence(boolean reverse)
		{
			int len = data[0];

			StringBuilder sb = new StringBuilder();
			
			for (int i = 0; i < len; i++)
				{
				int base = data[1 + (i >> 2)];

				base >>= (2 * (3 - (i & 0x3)));

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

			if(reverse)
				return sb.reverse().toString();
			else
				return sb.toString();
		}
		
		
		@Override
		public String toString() {
			return "Tail [seq="
					+ getTailSequence(false) + ", smerExists=" + smerExists
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
	
	
	public static class RouteContext
	{
		private int routeIndex;
		private int localPosition;
	}
	
	public static enum WhichTail
	{
		PREFIX(),
		SUFFIX();
		
		static WhichTail other(WhichTail tail)
		{
			return tail==PREFIX?SUFFIX:PREFIX;
		}
	}
	
	public static class Context
	{
		private LinkedSmer smer;
		private boolean compFlag;
		private int tailIndex;
		private WhichTail tail;
		private int tailPosition;
		
		public Context(LinkedSmer smer, boolean compFlag, int tailIndex, WhichTail tail, int tailPosition)
		{
			this.smer=smer;
			this.compFlag=compFlag;
			this.tailIndex=tailIndex;
			this.tail=tail;
			this.tailPosition=tailPosition;
			
			if(tailPosition<0)
				throw new RuntimeException();
		}
		
		/*
		public static Context makeReadBeginContext(LinkedSmer smer, WhichTail tail, int routeIndex, int localPosition)
		{
			Route route=smer.routes[routeIndex];
			
			if(tail==WhichTail.PREFIX)
				return new Context(smer, false, route.prefix, WhichTail.PREFIX, smer.prefixOffsetOfRoute(route.prefix, routeIndex, false)+localPosition);
			else
				return new Context(smer, true, route.suffix, WhichTail.SUFFIX, smer.suffixOffsetOfRoute(route.suffix, routeIndex, true)+localPosition);
		}
	*/
		public LinkedSmer getSmer() {
			return smer;
		}

		public boolean isCompFlag() {
			return compFlag;
		}

		public int getTailIndex() {
			return tailIndex;
		}

		public WhichTail isTail() {
			return tail;
		}
		
		public int getPositionIndex()
		{
			return tailPosition;
		}
	
		@Override
		public String toString() {
			return "Context [compFlag=" + compFlag + ", smer=" + smer.smerId
					+ ", tail=" + tail + ", tailIndex=" + tailIndex
					+ ", tailPosition=" + tailPosition + "]";
		}

		@Override
		public boolean equals(Object obj) {
			if (this == obj)
				return true;
			if (obj == null)
				return false;
			if (getClass() != obj.getClass())
				return false;
			Context other = (Context) obj;
			if (compFlag != other.compFlag)
				return false;
			if (smer.smerId != other.smer.smerId)
				return false;
			if (!tail.equals(other.tail))
				return false;
			if (tailIndex != other.tailIndex)
				return false;
			if (tailPosition != other.tailPosition)
				return false;
			return true;
		}

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
