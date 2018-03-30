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
