package logan.io;

public class FastqRecord
{
	private String name;
	private String sequence;
	private String comment;
	private String quality;
	
	private int phredOffset;
	private int headPos;
	
	public FastqRecord(String name, String sequence, String comment, String quality, int phredOffset)
	{
		this.name=name;
		this.sequence=sequence;
		this.comment=comment;
		this.quality=quality;
	
		this.phredOffset=phredOffset;
		headPos=0;
		
		if(sequence.length()!=quality.length())
			throw new RuntimeException("Sequence and quality length don't match: '"+sequence+"' vs '"+quality+"'");
	}
	
		
	public String getName()
	{
		return name;
	}

	public String getSequence()
	{
		return sequence;
	}
    
	public String getComment()
	{
		return comment;
	}

	public String getQuality()
	{
		return quality;
	}
	
	public int getPhredOffset()
	{
		return phredOffset;
	}
	
	void setPhredOffset(int phredOffset)
	{
		this.phredOffset=phredOffset;
	}
	
	public int getHeadPos()
	{
		return headPos;
	}
	

	public int[] getQualityAsInteger(boolean zeroNs)
	{
		int arr[]=new int[quality.length()];
		
		for(int i=0;i<quality.length();i++)
			{
			if(zeroNs && sequence.charAt(i)=='N')
				arr[i]=0;
			else
				arr[i]=quality.charAt(i)-phredOffset;
			}
			
		return arr;
	}
	
	
}
