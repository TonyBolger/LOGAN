package logan.graph;

import it.unimi.dsi.fastutil.ints.Int2ObjectMap;
import it.unimi.dsi.fastutil.ints.Int2ObjectOpenHashMap;
import logan.graph.LinkedSmer.WhichRouteTable;

public class TagIndex {

	Int2ObjectMap<TagIndexEntry> endMap;

	public TagIndex()
	{
		endMap=new Int2ObjectOpenHashMap<TagIndexEntry>();
	}

	public void registerEndTag(int taggedId, WhichRouteTable routeTable, int nodePosition)
	{
		endMap.put(taggedId, new TagIndexEntry(routeTable, nodePosition));
	}

	public TagIndexEntry getEnd(int taggedId)
	{
		return endMap.get(taggedId);
	}

	public int getEndCount()
	{
		return endMap.size();
	}

	static class TagIndexEntry{
		private WhichRouteTable routeTable;
		private int nodePosition;

		TagIndexEntry(WhichRouteTable routeTable, int nodePosition)
		{
			this.routeTable=routeTable;
			this.nodePosition=nodePosition;
		}

		public WhichRouteTable getRouteTable() {
			return routeTable;
		}

		public int getNodePosition() {
			return nodePosition;
		}



	}
}
