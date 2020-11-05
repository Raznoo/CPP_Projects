#ifndef SHARDING_SHARDMASTER_H
#define SHARDING_SHARDMASTER_H

#include "../common/common.h"

#include <grpcpp/grpcpp.h>
#include "../build/shardmaster.grpc.pb.h"

class StaticShardmaster : public Shardmaster::Service {
  using Empty = google::protobuf::Empty;

 public:
  // TODO implement these four methods!
  ::grpc::Status Join(::grpc::ServerContext* context,
                      const ::JoinRequest* request, Empty* response) override;
  ::grpc::Status Leave(::grpc::ServerContext* context,
                       const ::LeaveRequest* request, Empty* response) override;
  ::grpc::Status Move(::grpc::ServerContext* context,
                      const ::MoveRequest* request, Empty* response) override;
  ::grpc::Status Query(::grpc::ServerContext* context, const Empty* request,
                       ::QueryResponse* response) override;
    struct my_server {
        std::string address;
        std::vector<shard> server_shards;
        int space_needed;
        int base_lower;
        int base_upper;
    };

 private:
  // TODO add any fields you want here!
  // Hint: think about what sort of data structures make sense for keeping track
  // of which servers have which shards, as well as what kind of locking you
  // will need to ensure thread safety.
    std::mutex vect_mtx;
    std::vector<my_server> server_vect;
//    std::map<Shard, std::string> shard_map;
    int total_ind = MAX_KEY - MIN_KEY + 1;
};

#endif  // SHARDING_SHARDMASTER_H
