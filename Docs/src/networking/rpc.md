# RPC System

LAG's RPC (remote procedure call) system lets you register C++ functions to be
invoked by remote peers over any transport (UDP, ReliableUDP, WebSocket).

## Registering an RPC

```cpp
auto& session = /* your NetworkSession */;
auto& rpc = session.GetRPCRegistry();

uint32_t damageID = rpc.Register("apply_damage",
    RPCAuthority::Authority,     // server-only can call
    TransferMode::Reliable,
    [](PeerID sender, BinaryReader& args) {
        uint64_t entityID = args.ReadU64();
        float amount = args.ReadF32();
        // ... apply damage in the scene ...
    });
```

## Invoking

```cpp
auto payload = RPCRegistry::Serialize(damageID, uint64_t(playerID), 10.5f);
session.BroadcastRPC(damageID, payload, TransferMode::Reliable);
// or
session.SendRPC(peerID, damageID, payload);
```

## Authority modes

| Mode | Meaning |
|------|---------|
| `Authority` | Only the server may invoke; clients receive |
| `AnyPeer` | Any peer may invoke |
| `CallLocal` | Execute on sender *in addition to* remotes |

## Transfer modes

| Mode | Under the hood |
|------|----------------|
| `Reliable` | Retry + ack until acknowledged |
| `Unreliable` | Fire-and-forget — may drop |
| `UnreliableOrdered` | Drop out-of-order, deliver newest |

## Binary serialization

The `BinaryWriter` / `BinaryReader` pair handles all primitive types,
strings, and GLM vec/quat/mat. Extend with your own types by adding
`WriteArg` overloads in `RPCRegistry`.
