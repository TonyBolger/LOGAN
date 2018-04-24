
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

static s64 readPacketHeader(int fd, SerdesPacketHeader *header)
{
	s64 size=sizeof(SerdesPacketHeader);
	s64 headerBytes=read(fd, header, size);

	if(headerBytes!=size)
		{
		if(headerBytes!=0)
			LOG(LOG_CRITICAL,"Read Header: %li vs %li. Error %i", size, headerBytes, errno);

		return 1;
		}

	if(header->magic!=SERDES_PACKET_MAGIC)
		{
		LOG(LOG_CRITICAL,"Invalid Magic: %08x", header->magic);
		return 1;
		}

	if(header->version!=0)
		{
		LOG(LOG_CRITICAL,"Unexpected packet version: %08x", header->version);
		return 0;
		}

	return 0;
}


s64 serWriteSliceNodes(GraphSerdes *serdes, int fd, int sliceNo)
{
	SmerArray *smerArray=&(serdes->graph->smerArray);
	SmerArraySlice *slice=&(smerArray->slice[sliceNo]);

	s32 smerCount=slice->smerCount;
	s64 size=sizeof(SerdesPacketHeader)+sizeof(SerdesPacketNode)+sizeof(SmerEntry)*smerCount;

	u8 *data=dAlloc(serdes->disp, size);

	initPacketHeader(data, SERDES_PACKETTYPE_NODE, size);

	SerdesPacketNode *nodePacket=(SerdesPacketNode *)(data+sizeof(SerdesPacketHeader));

	nodePacket->sliceNo=sliceNo;
	nodePacket->smerCount=smerCount;

	memcpy(nodePacket->smerEntries, slice->smerIT, sizeof(SmerEntry)*smerCount);

	s64 bytes=write(fd, data, size);

	if(bytes!=size)
		LOG(LOG_CRITICAL,"Write Nodes: %li vs %li. Error %i", size, bytes, errno);

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



s64 serReadNodePacket(GraphSerdes *serdes, int fd)
{
	SerdesPacketHeader header;

	if(readPacketHeader(fd, &header))
		return 0;

	if(header.type!=SERDES_PACKETTYPE_NODE)
		{
		LOG(LOG_CRITICAL,"Unexpected packet type: %08x", header.type);
		return 0;
		}

	s32 remainingSize=header.size-sizeof(SerdesPacketHeader);
	SerdesPacketNode *packetNode=dAlloc(serdes->disp, remainingSize);
	s64 payloadBytes=read(fd, packetNode, remainingSize);

	if(payloadBytes!=remainingSize)
		LOG(LOG_CRITICAL,"Read Nodes: %li vs %li. Error %i", remainingSize, payloadBytes, errno);

	int sliceNum=packetNode->sliceNo;

	SmerArray *smerArray=&(serdes->graph->smerArray);

	saSetSliceSmerEntries(smerArray, sliceNum, packetNode->smerEntries, packetNode->smerCount);

	dispenserReset(serdes->disp);

	return payloadBytes;
}

s64 serReadNodes(GraphSerdes *serdes, int fd)
{
	s64 bytes=serReadNodePacket(serdes, fd);
	s64 totalBytes=bytes;

	while(bytes>0)
		{
		bytes=serReadNodePacket(serdes, fd);
		totalBytes+=bytes;
		}


	LOG(LOG_INFO,"Graph now contains %i Nodes", saGetSmerCount(&(serdes->graph->smerArray)));

	return totalBytes;
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
	  LOG(LOG_CRITICAL,"Write Edges: %li vs %li. Error %i", totalSize, bytes, errno);

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


s64 serReadEdgePacket(GraphSerdes *serdes, int fd)
{
	SerdesPacketHeader header;

	if(readPacketHeader(fd, &header))
		return 0;

	if(header.type!=SERDES_PACKETTYPE_EDGE)
		{
		LOG(LOG_CRITICAL,"Unexpected packet type: %08x", header.type);
		return 0;
		}

	s32 remainingSize=header.size-sizeof(SerdesPacketHeader);
	SerdesPacketEdge *packetEdge=dAlloc(serdes->disp, remainingSize);
	s64 payloadBytes=read(fd, packetEdge, remainingSize);

	if(payloadBytes!=remainingSize)
		LOG(LOG_CRITICAL,"Read Edges: %li vs %li. Error %i", remainingSize, payloadBytes, errno);

	int sliceNum=packetEdge->sliceNo;
	int smerCount=packetEdge->smerCount;

	SmerArray *smerArray=&(serdes->graph->smerArray);
	SmerArraySlice *slice=&(smerArray->slice[sliceNum]);

	u32 size=sizeof(SmerId)*smerCount;
	SmerId *smerIds=dAlloc(serdes->disp, size);

	saGetSliceSmerIds(slice, sliceNum, smerIds);

	u32 *sizeTable=(u32 *)(packetEdge->data);
	u64 sizeTableSize=sizeof(u32)*smerCount*2;
	u8 *data=packetEdge->data+sizeTableSize;

	u32 dataSize=0;

	for(int i=0;i<smerCount;i++)
		{
		SmerId smerId=smerIds[i];

		u32 prefixSize=*(sizeTable++);
		u32 suffixSize=*(sizeTable++);

		rtSetTailData(smerArray, smerId, data, prefixSize, data+prefixSize, suffixSize);

		u32 combinedSize=(prefixSize+suffixSize);

		data+=combinedSize;
		dataSize+=combinedSize;
		}

	u32 totalSize=sizeof(SerdesPacketEdge)+sizeTableSize+dataSize;

	if(remainingSize!=totalSize)
		LOG(LOG_CRITICAL,"Read Edges: Size Mismatch: %i vs %i", remainingSize, totalSize);

	dispenserReset(serdes->disp);

	return payloadBytes;
}

s64 serReadEdges(GraphSerdes *serdes, int fd)
{
	s64 bytes=serReadEdgePacket(serdes, fd);
	s64 totalBytes=bytes;

	while(bytes>0)
		{
		bytes=serReadEdgePacket(serdes, fd);
		totalBytes+=bytes;
		}

	return totalBytes;
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

		*(sizeTable++)=dataSize;
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

		*(sizeTable++)=dataSize;
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
		LOG(LOG_CRITICAL, "RouteTree %li vs %li. Error %i", totalSize, bytes, errno);

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
		LOG(LOG_CRITICAL, "RouteArray %li vs %li. Error %i", totalSize, bytes, errno);

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



s64 readSmerRouteTree(GraphSerdes *serdes, int fd, SerdesPacketHeader *header)
{
	SmerArray *smerArray=&(serdes->graph->smerArray);

	s32 remainingSize=header->size-sizeof(SerdesPacketHeader);
	SerdesPacketRouteTree *packetRouteTree=dAlloc(serdes->disp, remainingSize);
	s64 payloadBytes=read(fd, packetRouteTree, remainingSize);

	if(payloadBytes!=remainingSize)
		LOG(LOG_CRITICAL,"Read Route Array: %li vs %li. Error %i", remainingSize, payloadBytes, errno);

	SmerId smerId=packetRouteTree->smerId;
	int forwardLeafCount=packetRouteTree->forwardLeaves;
	int reverseLeafCount=packetRouteTree->reverseLeaves;

	u32 *sizeTable=(u32 *)(packetRouteTree->data);
	u64 sizeTableSize=sizeof(u32)*(forwardLeafCount+reverseLeafCount);
	u8 *data=packetRouteTree->data+sizeTableSize;

	u64 dataSize=rtSetRouteTableTreeData(smerArray, smerId, data, sizeTable, forwardLeafCount, reverseLeafCount, serdes->disp);

	u32 totalSize=sizeof(SerdesPacketRouteTree)+sizeTableSize+dataSize;

	if(remainingSize!=totalSize)
		LOG(LOG_CRITICAL,"Read Route Tree: Size Mismatch: %i vs %i", remainingSize, totalSize);

	dispenserReset(serdes->disp);

	return dataSize;

}


s64 readSliceRoutes(GraphSerdes *serdes, int fd, SerdesPacketHeader *header)
{
	s32 remainingSize=header->size-sizeof(SerdesPacketHeader);
	SerdesPacketRouteArray *packetRouteArray=dAlloc(serdes->disp, remainingSize);
	s64 payloadBytes=read(fd, packetRouteArray, remainingSize);

	if(payloadBytes!=remainingSize)
		LOG(LOG_CRITICAL,"Read Route Array: %li vs %li. Error %i", remainingSize, payloadBytes, errno);

	int sliceNum=packetRouteArray->sliceNo;
	int smerCount=packetRouteArray->smerCount;

	SmerArray *smerArray=&(serdes->graph->smerArray);
	SmerArraySlice *slice=&(smerArray->slice[sliceNum]);

	u32 size=sizeof(SmerId)*smerCount;
	SmerId *smerIds=dAlloc(serdes->disp, size);

	saGetSliceSmerIds(slice, sliceNum, smerIds);

	u32 *sizeTable=(u32 *)(packetRouteArray->data);
	u64 sizeTableSize=sizeof(u32)*smerCount;
	u8 *data=packetRouteArray->data+sizeTableSize;

	u32 dataSize=0;

	for(int i=0;i<smerCount;i++)
		{
		SmerId smerId=smerIds[i];
		u32 tableSize=*(sizeTable++);

		if(tableSize>0)
			rtSetRouteTableArrayData(smerArray, smerId, data, tableSize);

		data+=tableSize;
		dataSize+=tableSize;
		}

	u32 totalSize=sizeof(SerdesPacketRouteArray)+sizeTableSize+dataSize;

	if(remainingSize!=totalSize)
		LOG(LOG_CRITICAL,"Read Route Array: Size Mismatch: %i vs %i", remainingSize, totalSize);

	dispenserReset(serdes->disp);

	return totalSize;

}


s64 serReadRoutePacket(GraphSerdes *serdes, int fd)
{
	SerdesPacketHeader header;

	if(readPacketHeader(fd, &header))
		return 0;

	if(header.type==SERDES_PACKETTYPE_RARR)
		return readSliceRoutes(serdes, fd, &header);
	else if(header.type==SERDES_PACKETTYPE_RTRE)
		return readSmerRouteTree(serdes, fd, &header);
	else
		LOG(LOG_CRITICAL,"Unexpected packet type: %08x", header.type);
		return 0;

}

s64 serReadRoutes(GraphSerdes *serdes, int fd)
{
	s64 bytes=serReadRoutePacket(serdes, fd);
	s64 totalBytes=bytes;

	while(bytes>0)
		{
		bytes=serReadRoutePacket(serdes, fd);
		totalBytes+=bytes;
		}

	return totalBytes;
}



s64 serWriteNodesToFile(Graph *graph, char *filePath)
{
	LOG(LOG_INFO,"Writing Nodes to %s", filePath);
	int fd=open(filePath, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	s64 ret=serWriteNodes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
	return ret;
}

s64 serWriteEdgesToFile(Graph *graph, char *filePath)
{
	LOG(LOG_INFO,"Writing Edges to %s", filePath);
	int fd=open(filePath, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	s64 ret=serWriteEdges(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
	return ret;
}

s64 serWriteRoutesToFile(Graph *graph, char *filePath)
{
	LOG(LOG_INFO,"Writing Routes to %s", filePath);
	int fd=open(filePath, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	s64 ret=serWriteRoutes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
	return ret;
}

s64 serReadNodesFromFile(Graph *graph, char *filePath)
{
	LOG(LOG_INFO,"Reading Nodes from %s", filePath);

	int fd=open(filePath, O_RDONLY);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	u64 ret=serReadNodes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
	return ret;
}

s64 serReadEdgesFromFile(Graph *graph, char *filePath)
{
	LOG(LOG_INFO,"Reading Edges from %s", filePath);

	int fd=open(filePath, O_RDONLY);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	u64 ret=serReadEdges(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
	return ret;
}

s64 serReadRoutesFromFile(Graph *graph, char *filePath)
{
	LOG(LOG_INFO,"Reading Routes from %s", filePath);

	int fd=open(filePath, O_RDONLY);

	GraphSerdes serdes;
	serInitSerdes(&serdes, graph);

	u64 ret=serReadRoutes(&serdes, fd);

	serCleanupSerdes(&serdes);

	close(fd);
	return ret;
}






