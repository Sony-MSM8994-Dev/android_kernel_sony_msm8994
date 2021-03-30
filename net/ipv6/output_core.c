/*
 * IPv6 library code, needed by static components when full IPv6 support is
 * not configured or static.  These functions are needed by GSO/GRO implementation.
 */
#include <linux/export.h>
#include <net/ip.h>
#include <net/ipv6.h>
#include <net/ip6_fib.h>

/* This function exists only for tap drivers that must support broken
 * clients requesting UFO without specifying an IPv6 fragment ID.
 *
 * This is similar to ipv6_select_ident() but we use an independent hash
 * seed to limit information leakage.
 *
 * The network header must be set before calling this.
 */
void ipv6_proxy_select_ident(struct sk_buff *skb)
{
	static u32 ip6_proxy_idents_hashrnd __read_mostly;
	struct in6_addr buf[2];
	struct in6_addr *addrs;
	u32 hash, id;

	addrs = skb_header_pointer(skb,
				   skb_network_offset(skb) +
				   offsetof(struct ipv6hdr, saddr),
				   sizeof(buf), buf);
	if (!addrs)
		return;

	net_get_random_once(&ip6_proxy_idents_hashrnd,
			    sizeof(ip6_proxy_idents_hashrnd));

	hash = __ipv6_addr_jhash(&addrs[1], ip6_proxy_idents_hashrnd);
	hash = __ipv6_addr_jhash(&addrs[0], hash);

	id = ip_idents_reserve(hash, 1);
	skb_shinfo(skb)->ip6_frag_id = htonl(id);
}
EXPORT_SYMBOL_GPL(ipv6_proxy_select_ident);

int ip6_find_1stfragopt(struct sk_buff *skb, u8 **nexthdr)
{
	unsigned int offset = sizeof(struct ipv6hdr);
	unsigned int packet_len = skb->tail - skb->network_header;
	int found_rhdr = 0;
	*nexthdr = &ipv6_hdr(skb)->nexthdr;

	while (offset <= packet_len) {
		struct ipv6_opt_hdr *exthdr;
		unsigned int len;

		switch (**nexthdr) {

		case NEXTHDR_HOP:
			break;
		case NEXTHDR_ROUTING:
			found_rhdr = 1;
			break;
		case NEXTHDR_DEST:
#if IS_ENABLED(CONFIG_IPV6_MIP6)
			if (ipv6_find_tlv(skb, offset, IPV6_TLV_HAO) >= 0)
				break;
#endif
			if (found_rhdr)
				return offset;
			break;
		default :
			return offset;
		}

		if (offset + sizeof(struct ipv6_opt_hdr) > packet_len)
			return -EINVAL;

		exthdr = (struct ipv6_opt_hdr *)(skb_network_header(skb) +
						 offset);
		len = ipv6_optlen(exthdr);
		if (len + offset >= IPV6_MAXPLEN)
			return -EINVAL;
		offset += len;
		*nexthdr = &exthdr->nexthdr;
	}

	return -EINVAL;
}
EXPORT_SYMBOL(ip6_find_1stfragopt);
