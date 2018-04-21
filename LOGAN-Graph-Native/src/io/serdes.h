#ifndef __SERDES_H
#define __SERDES_H


typedef struct graphSerdesStr
{
	Graph *graph;

	MemDispenser *disp;
	MemDispenser *subDisp;
} GraphSerdes;

#define SERDES_PACKET_MAGIC 0x4E474F4C

#define SERDES_PACKETTYPE_NODE 0x45444F4E
#define SERDES_PACKETTYPE_EDGE 0x45474445

#define SERDES_PACKETTYPE_RARR 0x52524152
#define SERDES_PACKETTYPE_RTRE 0x45525452

typedef struct serdesPacketHeaderStr
{
	u32 magic;
	u32 type;
	u32 size;
	u32 version;
} SerdesPacketHeader;

typedef struct serdesPacketNodeStr
{
	u32 sliceNo;
	u32 smerCount;
	SmerEntry smerEntries[];
} SerdesPacketNode;


typedef struct serdesPacketEdgeStr
{
	u32 sliceNo;
	u32 smerCount;
	u8 data[]; // 2 x u32 per smer, indicating prefix/suffix size, followed by 'size' bytes for each (>0 length) tail
} SerdesPacketEdge;


typedef struct serdesPacketRouteArrayStr
{
	u32 sliceNo;
	u32 smerCount;
	u8 data[]; // u32 per smer, indicating forward/reverse table combined size, followed by 'size' bytes for each (>0 length) array-format table.
} SerdesPacketRouteArray;


typedef struct serdesPacketRouteTreeStr
{
	SmerId smerId;
	u32 forwardLeaves;
	u32 reverseLeaves;
	u8 data[]; // u32 per leaf, indicating leaf size, followed by 'size' bytes for each leaf.
} SerdesPacketRouteTree;



void serInitSerdes(GraphSerdes *serdes, Graph *graph);
void serCleanupSerdes(GraphSerdes *serdes);

s64 serWriteSliceNodes(GraphSerdes *serdes, int fd, int sliceNo);
s64 serWriteNodes(GraphSerdes *serdes, int fd);
s64 serReadNodes(GraphSerdes *serdes, int fd);

s64 serWriteSliceEdges(GraphSerdes *serdes, int fd, int sliceNo);
s64 serWriteEdges(GraphSerdes *serdes, int fd);
s64 serReadEdges(GraphSerdes *serdes, int fd);

s64 serWriteSliceRoutes(GraphSerdes *serdes, int fd, int sliceNo);
s64 serWriteRoutes(GraphSerdes *serdes, int fd);
s64 serReadRoutes(GraphSerdes *serdes, int fd);


s64 serWriteNodesToFile(Graph *graph, char *filePath);
s64 serWriteEdgesToFile(Graph *graph, char *filePath);
s64 serWriteRoutesToFile(Graph *graph, char *filePath);

s64 serReadNodesFromFile(Graph *graph, char *filePath);
s64 serReadEdgesFromFile(Graph *graph, char *filePath);
s64 serReadRoutesFromFile(Graph *graph, char *filePath);


#endif
