package logan.graph;

public class SequenceIndex {

	private SequenceSource sources[];

	public SequenceIndex(SequenceSource sources[])
	{
		this.sources=sources;
	}

	public SequenceSource[] getSources() {
		return sources;
	}

	public void dump()
	{
		//LOG(LOG_INFO,"Dump SequenceIndex contains %i Sources", seqIndex->sequenceSourceCount);

		System.out.println("SequenceIndex: Contains "+sources.length+" Sources");
		for(SequenceSource seqSrc: sources)
			{
			System.out.println("  SequenceSource "+seqSrc.name+" contains "+seqSrc.sequences.length+" Sequences");
			for(Sequence seq: seqSrc.sequences)
				{
				System.out.println("    Sequence "+seq.name+" ("+seq.totalLength+") contains "+seq.fragments.length+" Fragments");

				if(seq.fragments.length>=2)
					{
					SequenceFragment seqFrag=seq.fragments[0];
					System.out.println("      First Fragment ID "+seqFrag.sequenceId+" Range "+seqFrag.startPosition+" to "+seqFrag.endPosition);

					seqFrag=seq.fragments[seq.fragments.length-1];
					System.out.println("      Last Fragment ID "+seqFrag.sequenceId+" Range "+seqFrag.startPosition+" to "+seqFrag.endPosition);
					}

				//for(SequenceFragment seqFrag: seq.fragments)
//					System.out.println("      Fragment ID "+seqFrag.sequenceId+" Range "+seqFrag.startPosition+" to "+seqFrag.endPosition);
				}
			}
	}

	public void dumpShallow()
	{
			//LOG(LOG_INFO,"Dump SequenceIndex contains %i Sources", seqIndex->sequenceSourceCount);

			int fragmentCount=0;
			System.out.println("SequenceIndex: Contains "+sources.length+" Sources");
			for(SequenceSource seqSrc: sources)
				{
				System.out.println("  SequenceSource "+seqSrc.name+" contains "+seqSrc.sequences.length+" Sequences");
				for(Sequence seq: seqSrc.getSequences())
					fragmentCount+=seq.getFragments().length;
				}

			System.out.println("  Total Fragments: "+fragmentCount);
	}



	public static class SequenceSource
	{
		private String name;
		private Sequence[] sequences;

		public SequenceSource(String name, Sequence[] sequences)
		{
			this.name=name;
			this.sequences=sequences;
		}

		public String getName() {
			return name;
		}

		public Sequence[] getSequences() {
			return sequences;
		}


	}

	public static class Sequence
	{
		private String name;
		private SequenceFragment fragments[];
		private int totalLength;

		public Sequence(String name, SequenceFragment fragments[], int totalLength)
		{
			this.name=name;
			this.fragments=fragments;
			this.totalLength=totalLength;
		}

		public String getName() {
			return name;
		}

		public SequenceFragment[] getFragments() {
			return fragments;
		}

		public int getTotalLength() {
			return totalLength;
		}


	}

	public static class SequenceFragment
	{
		private int sequenceId;
		private int startPosition;
		private int endPosition;

		public SequenceFragment(int sequenceId, int startPosition, int endPosition)
		{
			this.sequenceId=sequenceId;
			this.startPosition=startPosition;
			this.endPosition=endPosition;
		}

		public int getSequenceId() {
			return sequenceId;
		}

		public int getStartPosition() {
			return startPosition;
		}

		public int getEndPosition() {
			return endPosition;
		}

	}
}
