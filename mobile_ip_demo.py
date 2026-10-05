# mobile_ip_demo.py
from scapy.layers.mobileip import MobileIP, MobileIPRRQ, MobileIPRRP, MobileIPTunnelData
from scapy.layers.inet import IP, UDP
from scapy.all import wrpcap

HA_IP  = "10.0.0.1"
MN_HOME = "10.0.0.5"
COA    = "192.168.2.5"   # Care-of Address (foreign network)

# ── 1. Registration Request (MN → HA) ──────────────────────────
rrq = (IP(src=COA, dst=HA_IP) /
       UDP(sport=434, dport=434) /
       MobileIP() /
       MobileIPRRQ(homeaddr=MN_HOME, haaddr=HA_IP,
                   coaddr=COA, lifetime=180, id=1001))

print("="*50)
print("PACKET 1: Registration Request (RRQ)")
print("="*50)
rrq.show()

# ── 2. Registration Reply success (HA → MN) ────────────────────
rrp_ok = (IP(src=HA_IP, dst=COA) /
           UDP(sport=434, dport=434) /
           MobileIP(type=3) /
           MobileIPRRP(code=0, homeaddr=MN_HOME,
                       haaddr=HA_IP, lifetime=180, id=1001))

print("\n" + "="*50)
print("PACKET 2: Registration Reply (RRP) — SUCCESS code=0")
print("="*50)
rrp_ok.show()

# ── 3. HA tunnels data to MN (IP-in-IP) ───────────────────────
inner_pkt = IP(src="172.16.0.5", dst=MN_HOME) / UDP() / b"Hello MN!"
tunnel = (IP(src=HA_IP, dst=COA) /
          MobileIPTunnelData(nexthdr=4) /
          inner_pkt)

print("\n" + "="*50)
print("PACKET 3: Tunneled Data (HA → FA → MN)")
print("="*50)
tunnel.show()

# ── 4. FAILURE CASE: HA down, RRP code=128 ────────────────────
rrp_fail = (IP(src=HA_IP, dst=COA) /
             UDP(sport=434, dport=434) /
             MobileIP(type=3) /
             MobileIPRRP(code=128, homeaddr=MN_HOME,
                         haaddr=HA_IP, lifetime=0, id=1002))

print("\n" + "="*50)
print("PACKET 4: Registration Reply — FAILURE code=128 (HA unavailable)")
print("="*50)
rrp_fail.show()

# ── Save all to pcap ──────────────────────────────────────────
all_pkts = [rrq, rrp_ok, tunnel, rrp_fail]
wrpcap("mobileip_demo.pcap", all_pkts)
print("\n✓ Saved: mobileip_demo.pcap  (open in Wireshark)")