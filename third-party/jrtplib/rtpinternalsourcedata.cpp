/*

  This file is a part of JRTPLIB
  Copyright (c) 1999-2004 Jori Liesenborgs

  Contact: jori@lumumba.luc.ac.be

  This library was developed at the "Expertisecentrum Digitale Media"
  (http://www.edm.luc.ac.be), a research center of the "Limburgs Universitair
  Centrum" (http://www.luc.ac.be). The library is based upon work done for 
  my thesis at the School for Knowledge Technology (Belgium/The Netherlands).

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
  IN THE SOFTWARE.

*/

#include "rtpinternalsourcedata.h"
#include "rtppacket.h"
#include <string.h>

#include "rtpdebug.h"

RTPInternalSourceData::RTPInternalSourceData(u_int32_t ssrc):RTPSourceData(ssrc)
{
}

RTPInternalSourceData::~RTPInternalSourceData()
{
}

// The following function should delete rtppack if necessary
int RTPInternalSourceData::ProcessRTPPacket(RTPPacket *rtppack,const RTPTime &receivetime,bool *stored)
{
	bool accept;
	double tsunit;
	
	*stored = false;
	
	if (timestampunit < 0) 
		tsunit = INF_GetEstimatedTimestampUnit();
	else
		tsunit = timestampunit;
	stats.ProcessPacket(rtppack,receivetime,tsunit,ownssrc,&accept);
	if (!accept)
		return 0;

	// Ok, packet accepted

	validated = true;
	
	if (!ownssrc) // for own ssrc these variables depend on the outgoing packets, not on the incoming
		issender = true;
	
	// Now, we can place the packet in the queue
	
	if (packetlist.empty())
	{
		*stored = true;
		packetlist.push_back(rtppack);
		return 0;
	}

	// find the right position to insert the packet
	
	std::list<RTPPacket*>::iterator it,start;
	bool done = false;
	u_int32_t newseqnr = rtppack->GetExtendedSequenceNumber();
	
	it = packetlist.end();
	--it;
	start = packetlist.begin();
	
	while (!done)
	{
		RTPPacket *p;
		u_int32_t seqnr;
		
		p = *it;
		seqnr = p->GetExtendedSequenceNumber();
		if (seqnr > newseqnr)
		{
			if (it != start)
				--it;
			else // we're at the start of the list
			{
				*stored = true;
				done = true;
				packetlist.push_front(rtppack);
			}
		}
		else if (seqnr < newseqnr) // insert after this packet
		{
			++it;
			packetlist.insert(it,rtppack);
			done = true;
			*stored = true;
		}
		else // they're equal !! Drop packet
		{
			done = true;
		}
	}
	
	return 0;
}

int RTPInternalSourceData::ProcessSDESItem(u_int8_t id,const u_int8_t *data,size_t itemlen,const RTPTime &receivetime,bool *cnamecollis)
{
	*cnamecollis = false;
	
	stats.SetLastMessageTime(receivetime);
	
	switch(id)
	{
	case RTCP_SDES_ID_CNAME:
		{
			size_t curlen;
			u_int8_t *oldcname;
			
			// NOTE: we're going to make sure that the CNAME is only set once.
			oldcname = SDESinf.GetCNAME(&curlen);
			if (curlen == 0)
			{
				// if CNAME is set, the source is validated
				SDESinf.SetCNAME(data,itemlen);
				validated = true;
			}
			else // check if this CNAME is equal to the one that is already present
			{
				if (curlen != itemlen)
					*cnamecollis = true;
				else
				{
					if (memcmp(data,oldcname,itemlen) != 0)
						*cnamecollis = true;
				}
			}
		}
		break;
	case RTCP_SDES_ID_NAME:
		{
			u_int8_t *oldname;
			size_t oldlen;

			oldname = SDESinf.GetName(&oldlen);
			if (oldlen == 0) // Name not set
				return SDESinf.SetName(data,itemlen);
		}
		break;
	case RTCP_SDES_ID_EMAIL:
		{
			u_int8_t *oldemail;
			size_t oldlen;

			oldemail = SDESinf.GetEMail(&oldlen);
			if (oldlen == 0)
				return SDESinf.SetEMail(data,itemlen);
		}
		break;
	case RTCP_SDES_ID_PHONE:
		return SDESinf.SetPhone(data,itemlen);
	case RTCP_SDES_ID_LOCATION:
		return SDESinf.SetLocation(data,itemlen);
	case RTCP_SDES_ID_TOOL:
		{
			u_int8_t *oldtool;
			size_t oldlen;

			oldtool = SDESinf.GetTool(&oldlen);
			if (oldlen == 0)
				return SDESinf.SetTool(data,itemlen);
		}
		break;
	case RTCP_SDES_ID_NOTE:
		stats.SetLastNoteTime(receivetime);
		return SDESinf.SetNote(data,itemlen);
	}
	return 0;
}

#ifdef RTP_SUPPORT_SDESPRIV

int RTPInternalSourceData::ProcessPrivateSDESItem(const u_int8_t *prefix,size_t prefixlen,const u_int8_t *value,size_t valuelen,const RTPTime &receivetime)
{
	int status;
	
	stats.SetLastMessageTime(receivetime);
	status = SDESinf.SetPrivateValue(prefix,prefixlen,value,valuelen);
	if (status == ERR_RTP_SDES_MAXPRIVITEMS)
		return 0; // don't stop processing just because the number of items is full
	return status;
}

#endif // RTP_SUPPORT_SDESPRIV

int RTPInternalSourceData::ProcessBYEPacket(const u_int8_t *reason,size_t reasonlen,const RTPTime &receivetime)
{
	if (byereason)
	{
		delete [] byereason;
		byereason = 0;
		byereasonlen = 0;
	}

	byetime = receivetime;
	byereason = new u_int8_t[reasonlen];
	if (byereason == 0)
		return ERR_RTP_OUTOFMEM;
	memcpy(byereason,reason,reasonlen);
	byereasonlen = reasonlen;
	receivedbye = true;
	stats.SetLastMessageTime(receivetime);
	return 0;
}

