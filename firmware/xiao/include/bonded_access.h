#pragma once
#include <array>
#include <cstdint>
#include <cstring>

namespace robo {
// One physically enrolled phone. Call under the BLE mutex. The BLE stack must
// separately verify Secure Connections + MITM + bonding before authenticate().
class BondedAccess {
 public:
  void restore(const uint8_t* address) {std::memcpy(owner_.data(),address,6);paired_=true;}
  bool open(uint32_t now) {if(paired_ || connected_ || repairing_)return false;opened_=now;window_=true;return true;}
  bool enrolling(uint32_t now) const {return !repairing_ && window_ && uint32_t(now-opened_)<120000;}
  bool repairing() const {return repairing_;}
  // Only an explicitly confirmed local HTTP operation starts recovery.
  // Keep the old owner until the BLE stack and persistent storage are cleared.
  bool beginRepair() {
    if(!paired_ || connected_ || repairing_)return false;
    repairing_=true;window_=false;return true;
  }
  bool finishRepair(bool cleared,uint32_t now) {
    if(!repairing_)return false;
    repairing_=false;
    if(!cleared)return false;
    owner_.fill(0);peer_.fill(0);paired_=authenticated_=false;
    opened_=now;window_=true;return true;
  }
  uint32_t remaining(uint32_t now) const {return enrolling(now)?120000-uint32_t(now-opened_):0;}
  bool allowsConnection(uint32_t now) const {
    // Link admission does not grant access. Reconnects must still prove their
    // identity through the encrypted authentication callback below.
    return !repairing_ && !connected_ && (paired_ || enrolling(now));
  }
  bool connected() const {return connected_;}
  void connect(const uint8_t* address) {std::memcpy(peer_.data(),address,6);connected_=true;authenticated_=false;}
  void disconnect() {connected_=authenticated_=false;}
  bool mayPair(uint32_t now) const {return !repairing_ && connected_ && (enrolling(now) || (paired_ && peer_==owner_));}
  bool authenticate(const uint8_t* address,bool strong,uint32_t now) {
    authenticated_=false;
    if(repairing_ || !strong || !connected_ || std::memcmp(peer_.data(),address,6))return false;
    if(paired_ && peer_==owner_) {authenticated_=true;return true;}
    if(paired_ || !enrolling(now))return false;
    owner_=peer_;paired_=authenticated_=true;window_=false;return true;
  }
  bool authorized() const {return connected_ && authenticated_;}
  bool paired() const {return paired_;}
  const uint8_t* owner() const {return owner_.data();}
 private:
  std::array<uint8_t,6> owner_{},peer_{};
  uint32_t opened_=0;
  bool window_=false,paired_=false,connected_=false,authenticated_=false,repairing_=false;
};
}
