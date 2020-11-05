#include <grpcpp/grpcpp.h>

#include "shardkv.h"

/**
 * This method is analogous to a hashmap lookup. A key is supplied in the
 * request and if its value can be found, we should either set the appropriate
 * field in the response Otherwise, we should return an error. An error should
 * also be returned if the server is not responsible for the specified key
 *
 * @param context - you can ignore this
 * @param request a message containing a key
 * @param response we store the value for the specified key here
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status ShardkvServer::Get(::grpc::ServerContext* context,
                                  const ::GetRequest* request,
                                  ::GetResponse* response) {
    std::unique_lock<std::mutex> guard(this->mtx);
    bool within_limits = false;
    for (int i = 0; i < this->server_shards.size(); i++){
        if (request->key() <= this->server_shards[i].upper &&
            request->key() >= this->server_shards[i].lower){
            within_limits = true;
        }
    }
    if (this->server_map.find(request->key()) == this->server_map.end()){
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Key is not valid");
    } else if (!within_limits){
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "No associated shard");
    } else {
        response->set_data(this->server_map[request->key()]);
        return ::grpc::Status::OK;
    }
}

/**
 * Insert the given key-value mapping into our store such that future gets will
 * retrieve it
 * If the item already exists, you must replace its previous value.
 * This function should error if the server is not responsible for the specified
 * key.
 *
 * @param context - you can ignore this
 * @param request A message containing a key-value pair
 * @param response An empty message, as we don't need to return any data
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status ShardkvServer::Put(::grpc::ServerContext* context,
                                  const ::PutRequest* request,
                                  Empty* response) {
    std::unique_lock<std::mutex> guard(this->mtx);
    bool within_limits = false;
    for (int i = 0; i < this->server_shards.size(); i++){
        if (request->key() <= this->server_shards[i].upper &&
            request->key() >= this->server_shards[i].lower){
            within_limits = true;
        }
    }
    if (this->server_map.find(request->key()) == this->server_map.end()){
        this->server_map[request->key()] = request->data();
    } else if (!within_limits){
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "No associated shard");
    } else{
        this->server_map[request->key()] = request->data();
    }
    return ::grpc::Status::OK;
}

/**
 * Appends the data in the request to whatever data the specified key maps to.
 * If the key is not mapped to anything, this method should be equivalent to a
 * put for the specified key and value. If the server is not responsible for the
 * specified key, this function should fail.
 *
 * @param context - you can ignore this
 * @param request A message containngi a key-value pair
 * @param response An empty message, as we don't need to return any data
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>"
 */
::grpc::Status ShardkvServer::Append(::grpc::ServerContext* context,
                                     const ::AppendRequest* request,
                                     Empty* response) {
    std::unique_lock<std::mutex> guard(this->mtx);
    bool within_limits = false;
    for (int i = 0; i < this->server_shards.size(); i++){
        if (request->key() <= this->server_shards[i].upper &&
            request->key() >= this->server_shards[i].lower){
            within_limits = true;
        }
    }
    if (this->server_map.find(request->key()) == this->server_map.end()){
        this->server_map[request->key()] = request->data();
    } else if (!within_limits){
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "No associated shard");
    }  else {
        this->server_map[request->key()] = this->server_map[request->key()] + request->data();
    }
    return ::grpc::Status::OK;
}

/**
 * This method is called in a separate thread on periodic intervals (see the
 * constructor in shardkv.h for how this is done). It should query the shardmaster
 * for an updated configuration of how shards are distributed. You should then
 * find this server in that configuration and look at the shards associated with
 * it. These are the shards that the shardmaster deems this server responsible
 * for. Check that every key you have stored on this server is one that the
 * server is actually responsible for according to the shardmaster. If this
 * server is no longer responsible for a key, you should find the server that
 * is, and call the Put RPC in order to transfer the key/value pair to that
 * server. You should not let the Put RPC fail. That is, the RPC should be
 * continually retried until success. After the put RPC succeeds, delete the
 * key/value pair from this server's storage. Think about concurrency issues like
 * potential deadlock as you write this function!
 *
 * @param stub a grpc stub for the shardmaster, which we use to invoke the Query
 * method!
 */
void ShardkvServer::QueryShardmaster(Shardmaster::Stub* stub) {
  Empty query;
  QueryResponse response;
  ::grpc::ClientContext cc;
  auto status = stub->Query(&cc, query, &response);
  if (status.ok()) {
    //zero out current shards
    this->server_shards.clear();
    for(ConfigEntry configEntry : response.config()){
        if (configEntry.server() == this->address){
            for (Shard old_shard : configEntry.shards()){
                shard newShard;
                newShard.lower = old_shard.lower();
                newShard.upper = old_shard.upper();
                this->server_shards.push_back(newShard);
            }
        }
    }
    std::map<int, std::string>::iterator it;
    for (it = this->server_map.begin(); it!=this->server_map.end(); it++){
        bool within_bounds = false;
        for (int i = 0; i < server_shards.size(); i++){
            if (it->first <= server_shards[i].upper &&
                it->first >= server_shards[i].lower){
                within_bounds = true;
            }
        }
        if (!within_bounds) {
            std::string target_server;
            bool outer_break = false;
            for (ConfigEntry configEntry : response.config()) {
                for (Shard test_shard : configEntry.shards()){
                    if (test_shard.upper() >= it->first &&
                        test_shard.lower() <= it->first){
                        target_server = configEntry.server();
                        outer_break = true;
                        break;
                    }
                }
                if (outer_break) {
                    break;
                }
            }
            auto new_stup = Shardkv::NewStub(grpc::CreateChannel(target_server, grpc::InsecureChannelCredentials()));
            ::PutRequest putRequest;
            ::grpc::ClientContext cc2;
            putRequest.set_key(it->first);
            putRequest.set_data(it->second);
            new_stup->Put(&cc2, putRequest, &query);
        }
    }

  } else {
    //maybe make an error here?
  }
}
