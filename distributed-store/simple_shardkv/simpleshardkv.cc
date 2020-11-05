#include "simpleshardkv.h"

/**
 * This method is analogous to a hashmap lookup. A key is supplied in the
 * request and if its value can be found, you should set the appropriate
 * field in the response. Otherwise, you should return an error.
 *
 * @param context - you can ignore this
 * @param request a message containing a key
 * @param response we store the value for the specified key here
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status SimpleShardkvServer::Get(::grpc::ServerContext* context,
                                        const ::GetRequest* request,
                                        ::GetResponse* response) {
    std::unique_lock<std::mutex> guard(this->mtx);
    if (this->server_map.find(request->key()) == this->server_map.end()){
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Key is not valid");
    } else {
        response->set_data(this->server_map[request->key()]);
        return ::grpc::Status::OK;
    }
}

/**
 * Insert the given key-value mapping into our store such that future gets will
 * retrieve it.
 * If the item already exists, you must replace its previous value.
 *
 * @param context - you can ignore this
 * @param request A message containing a key-value pair
 * @param response An empty message, as we don't need to return any data
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status SimpleShardkvServer::Put(::grpc::ServerContext* context,
                                        const ::PutRequest* request,
                                        Empty* response) {
    std::unique_lock<std::mutex> guard(this->mtx);
    if (this->server_map.find(request->key()) == this->server_map.end()){
        this->server_map[request->key()] = request->data();
    } else{
        this->server_map[request->key()] = request->data();
    }
    return ::grpc::Status::OK;
}

/**
 * Appends the data in the request to whatever data the specified key maps to.
 * If the key is not mapped to anything, this method should be equivalent to a
 * put for the specified key and value.
 *
 * @param context - you can ignore this
 * @param request A message containing a key-value pair
 * @param response An empty message, as we don't need to return any data
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status SimpleShardkvServer::Append(::grpc::ServerContext* context,
                                           const ::AppendRequest* request,
                                           Empty* response) {
    std::unique_lock<std::mutex> guard(this->mtx);
    if (this->server_map.find(request->key()) == this->server_map.end()){
        this->server_map[request->key()] = request->data();
    } else {
        this->server_map[request->key()] = this->server_map[request->key()] + request->data();
    }
    return ::grpc::Status::OK;
}
