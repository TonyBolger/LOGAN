package logan.tools;

import java.io.File;
import java.io.IOException;

import logan.graph.Graph;
import logan.graph.GraphConfig;
import logan.graph.GraphReader;
import logan.io.FastqParser;
import logan.io.FastqRecord;

public class FastqMapper {

	public FastqMapper()
	{
	}
	
	private Graph readGraph(String graphBase)
	{
		
		GraphReader reader=new GraphReader(new GraphConfig());
	
		reader.readNodes(new File(graphBase+".nodes"));
		reader.readEdges(new File(graphBase+".routes"));
		reader.readRoutes(new File(graphBase+".routes"));
		
		Graph graph=reader.getGraph();
		
		return graph;
	}
	
	public void process(String graphBase, File fastqFile) throws IOException
	{
		Graph graph=readGraph(graphBase);
		
		FastqParser parser=new FastqParser(0);
		
		while(parser.hasNext())
		{
			FastqRecord rec=parser.next();
			
			
			
		}
		
		
	}
	
	
	public static void main(String[] args) {

		if(args.length!=2)
			{
			System.err.println("Usage: FastqMapper <graphPath> <fastq>");
			System.exit(1);
			}
		
		String graphBase=args[0];
		File fastqFile=new File(args[1]);
		
		FastqMapper mapper=new FastqMapper();
		mapper.process(graphBase,  fastqFile);

	}

}
