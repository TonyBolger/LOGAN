
#include "common.h"




void serInitSerdes(GraphSerdes *serdes, Graph *graph)
{
	if(graph!=NULL && graph->mode==GRAPH_MODE_INDEX)
		LOG(LOG_CRITICAL,"Graph cannot be in indexing mode");

	serdes->graph=graph;
	serdes->disp=dispenserAlloc(MEMTRACKID_SERDES, DISPENSER_BLOCKSIZE_LARGE, DISPENSER_BLOCKSIZE_HUGE);
	serdes->subDisp=dispenserAlloc(MEMTRACKID_SERDES, DISPENSER_BLOCKSIZE_LARGE, DISPENSER_BLOCKSIZE_HUGE);
}


void serCleanupSerdes(GraphSerdes *serdes)
{
	serdes->graph=NULL;
	dispenserFree(serdes->disp);
	dispenserFree(serdes->subDisp);
}




static u64 wrapped_writev(int fd, struct iovec *iov, int iovcnt)
{
	s64 total=0;

	struct iovec *iovEnd=iov+iovcnt;

	while(iov<iovEnd)
		{
		int iovNext=MIN(iovcnt, 1024);

		s64 bytes=writev(fd, iov, iovNext);

		if(bytes<0)
			break;

		total+=bytes;
		iov+=iovNext;
		iovcnt-=iovNext;
		}

	return total;
}


static void initPacketHeader(u8 *data, u32 type, u32 size)
{
	SerdesPacketHeader *header=(SerdesPacketHeader *)data;

	header->magic=SERDES_PACKET_MAGIC;
	header->type=type;
	header->size=size;
	header->version=0;
}

s64 serWriteSliceNodes(GraphSerdes *serdes, int fd, int sliceNo)
{
	SmerArray *smerArray=&(serdes->graph->smerArray);
	SmerArraySlice *slice=&(smerArray->slice[sliceNo]);

	s32 smerCount=slice->smerCount;
	s64 size=sizeof(SerdesPacketHeader)+sizeof(SerdesPacketNode)+sizeof(SmerId)*smerCount;

	u8 *data=dAlloc(serdes->disp, size);

	initPacketHeader(data, SERDES_PACKETTYPE_NODE, size);

	SerdesPacketNode *nodePacket=(SerdesPacketNode *)(data+sizeof(SerdesPacketHeader));

	nodePacket->sliceNo=sliceNo;
	nodePacket->smerCount=smerCount;
	saGetSliceSmerIds(slice, sliceNo, nodePacket->smers);

	s64 bytes=write(fd, data, size);

	if(bytes!=size)
		LOG(LOG_CRITICAL,"Nodes %li vs %li", size, bytes);

	dispenserReset(serdes->disp);

	return bytes;
}


s64 serWriteNodes(GraphSerdes *serdes, int fd)
{
	s64 total=0;

	for(int i=0;i<SMER_SLICES; i++)
		total+=serWriteSliceNodes(serdes, fd, i);

	return total;
}


s64 serWriteSliceEdges(GraphSerdes *serdes, int fd, int sliceNo)
{
	SmerArray *smerArray=&(serdes->graph->smerArray);
	SmerArraySlice *slice=&(smerArray->slice[sliceNo]);

	s32 smerCount=slice->smerCount;
	u32 size=sizeof(SmerId)*smerCount;
	SmerId *smerIds=dAlloc(serdes->disp, size);

	saGetSliceSmerIds(slice, sliceNo, smerIds);

	u64 headerSize=sizeof(SerdesPacketHeader)+sizeof(SerdesPacketEdge);
	u8 *headerData=dAlloc(serdes->disp, headerSize);

	SerdesPacketEdge *edgePacket=(SerdesPacketEdge *)(headerData+sizeof(SerdesPacketHeader));

	edgePacket->sliceNo=sliceNo;
	edgePacket->smerCount=smerCount;

	u64 sizeTableSize=sizeof(u32)*smerCount*2;
	u32 *sizeTableBase=dAlloc(serdes->disp, sizeTableSize);
	u32 *sizeTable=sizeTableBase;

	struct iovec *iovBase=dAlloc(serdes->disp, sizeof(struct iovec)*(2+smerCount*2)); // header, size table and prefix/suffix per smer
	struct iovec *iov=iovBase;

	iov->iov_base=headerData;
	iov->iov_len=headerSize;

	iov++;

	iov->iov_base=sizeTable;
	iov->iov_len=sizeTableSize;

	iov++;

	u64 totalSize=headerSize+sizeTableSize;

	for(int i=0;i<smerCount;i++)
		{
		u8 *prefixData;
		u32 prefixDataSize;
		u8 *suffixData;
		u32 suffixDataSize;

		rtGetTailData(smerArray, smerIds[i], &prefixData, &prefixDataSize, &suffixData, &suffixDataSize);

		*(sizeTable++)=prefixDataSize;
		if(prefixDataSize>0)
			{
			iov->iov_base=prefixData;
			iov->iov_len=prefixDataSize;
			iov++;

			totalSize+=prefixDataSize;
			}

		*(sizeTable++)=suffixDataSize;
		if(suffixDataSize>0)
			{
			iov->iov_base=suffixData;
			iov->iov_len=suffixDataSize;
			iov++;

			totalSize+=suffixDataSize;
			}
		}


	initPacketHeader(headerData, SERDES_PACKETTYPE_EDGE, totalSize);

	s64 bytes=wrapped_writev(fd, iovBase, iov-iovBase);

	if(bytes!=totalSize)
	  LOG(LOG_CRITICAL,"Edges %li vs %li. Error %i", totalSize, bytes, errno);

	dispenserReset(serdes->disp);

	return bytes;
}


s64 serWriteEdges(GraphSerdes *serdes, int fd)
{
	s64 total=0;

	for(int i=0;i<SMER_SLICES; i++)
		total+=serWriteSliceEdges(serdes, fd, i);

	return total;
}



static s64 writeSmerRouteTree(GraphSerdes *serdes, int fd, SmerArray *smerArray, SmerId smerId)
{
	u32 forwardCount=0;
	u8 **forwardData;
	u32 *forwardDataSize;

	u32 reverseCount=0;
	u8 **reverseData;
	u32 *reverseDataSize;

	rtGetRouteTableTreeData(smerArray, smerId, &forwardCount, &forwardData, &forwardDataSize, &reverseCount, &reverseData, &reverseDataSize, serdes->subDisp);

	u64 headerSize=sizeof(SerdesPacketHeader)+sizeof(SerdesPacketRouteTree);
	u8 *headerData=dAlloc(serdes->disp, headerSize);

	SerdesPacketRouteTree *routeTreePacket=(SerdesPacketRouteTree *)(headerData+sizeof(SerdesPacketHeader));

	routeTreePacket->smerId=smerId;
	routeTreePacket->forwardLeaves=forwardCount;
	routeTreePacket->reverseLeaves=reverseCount;

	u64 sizeTableSize=sizeof(u32)*(forwardCount+reverseCount);
	u32 *sizeTableBase=dAlloc(serdes->disp, sizeTableSize);
	u32 *sizeTable=sizeTableBase;

	struct iovec *iovBase=dAlloc(serdes->disp, sizeof(struct iovec)*(2+forwardCount+reverseCount)); // header, size table and combined table per smer
	struct iovec *iov=iovBase;

	iov->iov_base=headerData;
	iov->iov_len=headerSize;
	iov++;

	iov->iov_base=sizeTable;
	iov->iov_len=sizeTableSize;
	iov++;

	u64 totalSize=headerSize+sizeTableSize;

	for(int i=0;i<forwardCount;i++)
		{
		u8 *data=forwardData[i];
		u32 dataSize=forwardDataSize[i];

		if(dataSize>0)
			{
			iov->iov_base=data;
			iov->iov_len=dataSize;
			iov++;

			totalSize+=dataSize;
			}
		}

	for(int i=0;i<reverseCount;i++)
		{
		u8 *data=reverseData[i];
		u32 dataSize=reverseDataSize[i];

		if(dataSize>0)
			{
			iov->iov_base=data;
			iov->iov_len=dataSize;
			iov++;

			totalSize+=dataSize;
			}
		}

	initPacketHeader(headerData, SERDES_PACKETTYPE_RTRE, totalSize);

	s64 bytes=wrapped_writev(fd, iovBase, iov-iovBase);

	if(bytes!=totalSize)
		LOG(LOG_CRITICAL, "RouteTree %li vs %li", totalSize, bytes);

	dispenserReset(serdes->subDisp);

	return bytes;

}

s64 serWriteSliceRoutes(GraphSerdes *serdes, int fd, int sliceNo)
{
	SmerArray *smerArray=&(serdes->graph->smerArray);
	SmerArraySlice *slice=&(smerArray->slice[sliceNo]);

	s32 smerCount=slice->smerCount;
	u32 size=sizeof(SmerId)*smerCount;
	SmerId *smerIds=dAlloc(serdes->disp, size);

	saGetSliceSmerIds(slice, sliceNo, smerIds);

	u64 headerSize=sizeof(SerdesPacketHeader)+sizeof(SerdesPacketRouteArray);
	u8 *headerData=dAlloc(serdes->disp, headerSize);

	SerdesPacketRouteArray *routeArrayPacket=(SerdesPacketRouteArray *)(headerData+sizeof(SerdesPacketHeader));

	routeArrayPacket->sliceNo=sliceNo;
	routeArrayPacket->smerCount=smerCount;

	u64 sizeTableSize=sizeof(u32)*smerCount;
	u32 *sizeTableBase=dAlloc(serdes->disp, sizeTableSize);
	u32 *sizeTable=sizeTableBase;

	struct iovec *iovBase=dAlloc(serdes->disp, sizeof(struct iovec)*(2+smerCount)); // header, size table and combined table per smer
	struct iovec *iov=iovBase;

	iov->iov_base=headerData;
	iov->iov_len=headerSize;
	iov++;

	iov->iov_base=sizeTable;
	iov->iov_len=sizeTableSize;
	iov++;

	u64 totalSize=headerSize+sizeTableSize;

	u64 subBytes=0;

	for(int i=0;i<smerCount;i++)
		{
		u8 *data;
		u32 dataSize;

		int isTree=rtGetRouteTableArrayData(smerArray, smerIds[i], &data, &dataSize);

		*(sizeTable++)=dataSize;
		if(dataSize>0)
			{
			iov->iov_base=data;
			iov->iov_len=dataSize;
			iov++;

			totalSize+=dataSize;
			}

		if(isTree)
			subBytes+=writeSmerRouteTree(serdes,fd, smerArray, smerIds[i]);

		}

	initPacketHeader(headerData, SERDES_PACKETTYPE_RARR, totalSize);

	s64 bytes=wrapped_writev(fd, iovBase, iov-iovBase);

	if(bytes!=totalSize)
		LOG(LOG_CRITICAL, "RouteArray %li vs %li", totalSize, bytes);

	dispenserReset(serdes->disp);

	return bytes+subBytes;

}


s64 serWriteRoutes(GraphSerdes *serdes, int fd)
{
	s64 total=0;

	for(int i=0;i<SMER_SLICES; i++)
		total+=serWriteSliceRoutes(serdes, fd, i);

	return total;
}





