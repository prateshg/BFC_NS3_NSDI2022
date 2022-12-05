#ifndef TRACE_FORMAT_H
#define TRACE_FORMAT_H
#include <stdint.h>
#include <vector>
namespace ns3{

enum Event{
	Recv = 0,
	Enqu = 1,
	Dequ = 2
};
struct TraceFormat{
	uint64_t time;
	uint16_t node;
	uint8_t intf, qidx;
	uint32_t qlen;
	uint32_t sip, dip;
	uint16_t size;
	uint8_t l3Prot;
	uint8_t event;
	uint8_t ecn; // this is the ip ECN bits
	union{
		struct {
			uint16_t sport, dport;
			uint32_t seq;
			uint64_t ts;
			uint16_t pg;
			uint32_t endseq;
			uint32_t hqCnt_qCnt;
		} data;
		struct {
			uint16_t fid;
			uint8_t qIndex;
			uint16_t qfb;
			uint8_t ecnBits; // this is the ECN bits in the CNP
			uint16_t total;
		} cnp;
		struct {
			uint16_t pg;
			uint16_t port;
			uint32_t seq;
		} ack;
		struct {
			uint32_t time;
			uint16_t num;
			uint16_t pause_map[100];
			/*uint64_t pause_map_a;
			uint64_t pause_map_b;
			uint64_t pause_map_c;
			uint64_t pause_map_d;
			uint64_t pause_map_e;
			uint64_t pause_map_f;
			uint64_t pause_map_g;
			uint64_t pause_map_h;
			uint64_t pause_map_i;
			uint64_t pause_map_j;
			uint64_t pause_map_k;
			uint64_t pause_map_l;
			uint64_t pause_map_m;
			uint64_t pause_map_n;
			uint64_t pause_map_o;
			uint64_t pause_map_p;*/
		} pfc;
	};
};

static inline const char* EventToStr(enum Event e){
	switch (e){
		case Recv:
			return "Recv";
		case Enqu:
			return "Enqu";
		case Dequ:
			return "Dequ";
		default:
			return "????";
	}
}

}
#endif
