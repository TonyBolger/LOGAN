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
	SmerId smers[];
} SerdesPacketNode;


typedef struct serdesPacketEdgeStr
{
	u32 sliceNo;
	u32 smerCount;
	u8 *data[]; // 2 x u32 per smer, indicating prefix/suffix size, followed by 'size' bytes for each (>0 length) tail
} SerdesPacketEdge;


typedef struct serdesPacketRouteArrayStr
{
	u32 sliceNo;
	u32 smerCount;
	u8 *data[]; // u32 per smer, indicating forward/reverse table combined size, followed by 'size' bytes for each (>0 length) array-format table.
} SerdesPacketRouteArray;


typedef struct serdesPacketRouteTreeStr
{
	SmerId smerId;
	u32 forwardLeaves;
	u32 reverseLeaves;
	u8 *data[]; // u32 per leaf, indicating leaf size, followed by 'size' bytes for each leaf.
} SerdesPacketRouteTree;



/*

 Serialize/Deserialize the graph elements to packetized files. Each packet covers one slice (16384 slices total).

 Packet Header:
 	 u32 magic; // LOGN
 	 u32 type; // NODE / EDGE / ROUT

 	 u32 size;
 	 u32 version;

 Node Packet:
	Header
	u32 sliceNumber
	u32 smerCount
	u32 smers

 Edge Packet:
	TDB

 Route Packet:
	TDB

 */



void serInitSerdes(GraphSerdes *serdes, Graph *graph);
void serCleanupSerdes(GraphSerdes *serdes);

s64 serWriteSliceNodes(GraphSerdes *serdes, int fd, int sliceNo);
s64 serWriteNodes(GraphSerdes *serdes, int fd);

s64 serWriteSliceEdges(GraphSerdes *serdes, int fd, int sliceNo);
s64 serWriteEdges(GraphSerdes *serdes, int fd);

s64 serWriteSliceRoutes(GraphSerdes *serdes, int fd, int sliceNo);
s64 serWriteRoutes(GraphSerdes *serdes, int fd);




#endif
