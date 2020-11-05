#include "shardmaster.h"

/**
 * Based on the server specified in JoinRequest, you should update the
 * shardmaster's internal representation that this server has joined. Remember,
 * you get to choose how to represent everything the shardmaster tracks in
 * shardmaster.h! Be sure to rebalance the shards in equal proportions to all
 * the servers. This function should fail if the server already exists in the
 * configuration.
 *
 * @param context - you can ignore this
 * @param request A message containing the address of a key-value server that's
 * joining
 * @param response An empty message, as we don't need to return any data
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status StaticShardmaster::Join(::grpc::ServerContext* context,
                                       const ::JoinRequest* request,
                                       Empty* response) {
    //check to see if server in vector
    std::unique_lock<std::mutex> guard(this->vect_mtx);
    int total_serv = this->server_vect.size();
    for (int i = 0; i < total_serv; i++){
        if (this->server_vect[i].address == request->server()){
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Server Already Exists");
        }
    }
    //zero everything
    for (int i = 0; i < total_serv; i ++){
        this->server_vect[i].space_needed = 0;
        this->server_vect[i].server_shards.clear();
    }
//    this->shard_map.clear();
    //make a new server
    my_server newServ;
    newServ.address = request->server();
    //add the blank server to the array
    this->server_vect.push_back(newServ);
    //increment total server count
    total_serv++;
    //how many keys each server should have
    int server_key_size = this->total_ind / total_serv;
    //figure out how many extra keys I have to distribute
    int total_extra_keys = this->total_ind % server_key_size;
    //add the extra keys
    while (total_extra_keys > 0){
        for (int i = 0; i < total_serv; i++){
            this->server_vect[i].space_needed++;
            total_extra_keys--;
            if (total_extra_keys == 0){
                break;
            }
        }
    }
    //make a new shard based off upper and lower limits
    for (int i = 0; i < total_serv; i ++){
        //set real space (extra space + default space)
        this->server_vect[i].space_needed += server_key_size;
        //set lower limit
        if (i == 0){
            this->server_vect[i].base_lower = MIN_KEY;
        } else{
            this->server_vect[i].base_lower = this->server_vect[i - 1].base_upper + 1;
        }
        //set upper limit
        if (i == total_serv - 1){
            this->server_vect[i].base_upper = MAX_KEY;
        } else {
            this->server_vect[i].base_upper = this->server_vect[i].base_lower + this ->server_vect[i].space_needed - 1;
        }
        shard new_shard;
        new_shard.lower = (this->server_vect[i].base_lower);
        new_shard.upper = (this->server_vect[i].base_upper);
        this->server_vect[i].server_shards.push_back(new_shard);
    }
    return ::grpc::Status::OK;
}

/**
 * LeaveRequest will specify a list of servers leaving. This will be very
 * similar to join, wherein you should update the shardmaster's internal
 * representation to reflect the fact the server(s) are leaving. Once that's
 * completed, be sure to rebalance the shards in equal proportions to the
 * remaining servers. If any of the specified servers do not exist in the
 * current configuration, this function should fail.
 *
 * @param context - you can ignore this
 * @param request A message containing a list of server addresses that are
 * leaving
 * @param response An empty message, as we don't need to return any data
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status StaticShardmaster::Leave(::grpc::ServerContext* context,
                                        const ::LeaveRequest* request,
                                        Empty* response) {
    std::unique_lock<std::mutex> guard(this->vect_mtx);
    bool isIn = false;
    int total_serv = this ->server_vect.size();
    for(std::string address : request->servers()){
        for (int i = 0; i < total_serv; i++){
            if (this->server_vect[i].address == address){
                isIn = true;
            }
        }
        if (!isIn){
            return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Server Doesn't Exist");
        } else {
            isIn = false;
        }
    }
    for(std::string address : request->servers()) {
        for (int i = 0; i < total_serv; i++) {
            if (this->server_vect[i].address == address) {
                this->server_vect.erase(this->server_vect.begin() + i);
                total_serv--;
                break;
            }
        }
    }
    //zero everything
    for (int i = 0; i < total_serv; i ++){
        this->server_vect[i].space_needed = 0;
        this->server_vect[i].server_shards.clear();
    }
    int server_key_size = this->total_ind / total_serv;
    //figure out how many extra keys I have to distribute
    int total_extra_keys = this->total_ind % server_key_size;
    //add the extra keys
    while (total_extra_keys > 0){
        for (int i = 0; i < total_serv; i++){
            this->server_vect[i].space_needed++;
            total_extra_keys--;
            if (total_extra_keys == 0){
                break;
            }
        }
    }
    //make a new shard based off upper and lower limits
    for (int i = 0; i < total_serv; i ++){
        //set real space (extra space + default space)
        this->server_vect[i].space_needed += server_key_size;
        //set lower limit
        if (i == 0){
            this->server_vect[i].base_lower = MIN_KEY;
        } else{
            this->server_vect[i].base_lower = this->server_vect[i - 1].base_upper + 1;
        }
        //set upper limit
        if (i == total_serv - 1){
            this->server_vect[i].base_upper = MAX_KEY;
        } else {
            this->server_vect[i].base_upper = this->server_vect[i].base_lower + this ->server_vect[i].space_needed - 1;
        }
        shard new_shard;
        new_shard.lower = (this->server_vect[i].base_lower);
        new_shard.upper = (this->server_vect[i].base_upper);
        this->server_vect[i].server_shards.push_back(new_shard);
    }
    return ::grpc::Status::OK;
}

/**
 * Move the specified shard to the target server (passed in MoveRequest) in the
 * shardmaster's internal representation of which server has which shard. Note
 * this does not transfer any actual data in terms of kv-pairs. This function is
 * responsible for just updating the internal representation, meaning whatever
 * you chose as your data structure(s).
 *
 * @param context - you can ignore this
 * @param request A message containing a destination server address and the
 * lower/upper bounds of a shard we're putting on the destination server.
 * @param response An empty message, as we don't need to return any data
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status StaticShardmaster::Move(::grpc::ServerContext* context,
                                       const ::MoveRequest* request,
                                       Empty* response) {
    std::unique_lock<std::mutex> guard(this->vect_mtx);
    if (request->shard().upper() > MAX_KEY ||
        request->shard().lower() < MIN_KEY){
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Shard out of bounds");
    }
    bool isIn = false;
    for (int i = 0; i < this->server_vect.size(); i ++){
        if (this->server_vect[i].address == request->server()){
            isIn = true;
            break;
        }
    }
    if (!isIn){
        return ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "Server does not exist");
    }
    //make fake shard
    shard fake_shard;
    fake_shard.lower = request->shard().lower();
    fake_shard.upper = request->shard().upper();
    //find starting index
    //keep looping through servers until you get to the end of the servers
    for (int i = 0; i < this->server_vect.size(); i++){
        //classify each of the shards in the server
        for (int j = 0; j < this->server_vect[i].server_shards.size();){
            switch (get_overlap(this->server_vect[i].server_shards[j], fake_shard)){
                //end of multi overlap
                case OverlapStatus::OVERLAP_END :
                    shard new_front;
                    new_front.lower = this->server_vect[i].server_shards[j].lower;
                    new_front.upper = fake_shard.lower - 1;
                    this->server_vect[i].server_shards.erase(this->server_vect[i].server_shards.begin() + j);
                    this->server_vect[i].server_shards.push_back(new_front);
                    continue;
                    //beginning of multi overlap
                case OverlapStatus::OVERLAP_START :
                    shard new_end;
                    new_end.lower = fake_shard.upper + 1;
                    new_end.upper = this->server_vect[i].server_shards[j].upper;
                    this->server_vect[i].server_shards.erase(this->server_vect[i].server_shards.begin() + j);
                    this->server_vect[i].server_shards.push_back(new_end);
                    continue;
                //beginning and end of single overlap
                case OverlapStatus::COMPLETELY_CONTAINS :
                    //annihilate front
                    if (this->server_vect[i].server_shards[j].lower == fake_shard.lower) {
                        shard new_end;
                        new_end.lower = fake_shard.upper + 1;
                        new_end.upper = this->server_vect[i].server_shards[j].upper;
                        this->server_vect[i].server_shards.erase(this->server_vect[i].server_shards.begin() + j);
                        this->server_vect[i].server_shards.push_back(new_end);
                    }
                        //annihilate back
                    else if (this->server_vect[i].server_shards[j].upper == fake_shard.upper) {
                        shard new_front;
                        new_front.lower = this->server_vect[i].server_shards[j].lower;
                        new_front.upper = fake_shard.lower - 1;
                        this->server_vect[i].server_shards.erase(this->server_vect[i].server_shards.begin() + j);
                        this->server_vect[i].server_shards.push_back(new_front);
                    }
                        //remove middle
                    else {
                        shard new_front;
                        new_front.lower = this->server_vect[i].server_shards[j].lower;
                        new_front.upper = fake_shard.lower - 1;
                        shard new_end;
                        new_end.lower = fake_shard.upper + 1;
                        new_end.upper = this->server_vect[i].server_shards[j].upper;
                        this->server_vect[i].server_shards.erase(this->server_vect[i].server_shards.begin() + j);
                        this->server_vect[i].server_shards.push_back(new_front);
                        this->server_vect[i].server_shards.push_back(new_end);
                    }
                    continue;
                //middle of multi overlap OR big single overlap
                case OverlapStatus::COMPLETELY_CONTAINED :
                    this->server_vect[i].server_shards.erase(this->server_vect[i].server_shards.begin() + j);
                    continue;
                //nothing in common
                default:
                    j++;
            }
        }
    }
    for (int i = 0; i < this->server_vect.size(); i++){
        if (this->server_vect[i].address == request->server()){
            this->server_vect[i].server_shards.push_back(fake_shard);
            break;
        }
    }
    return ::grpc::Status::OK;
    // Hint: Take a look at get_overlap in common.{h, cc}
  // Using the function will save you lots of time and effort!
}

/**
 * When this function is called, you should store the current servers and their
 * corresponding shards in QueryResponse. Take a look at
 * 'protos/shardmaster.proto' to see how to set QueryResponse correctly. Note
 * that its a list of ConfigEntry, which is a struct that has a server's address
 * and a list of the shards its currently responsible for.
 *
 * @param context - you can ignore this
 * @param request An empty message, as we don't need to send any data
 * @param response A message that specifies which shards are on which servers
 * @return ::grpc::Status::OK on success, or
 * ::grpc::Status(::grpc::StatusCode::INVALID_ARGUMENT, "<your error message
 * here>")
 */
::grpc::Status StaticShardmaster::Query(::grpc::ServerContext* context,
                                        const StaticShardmaster::Empty* request,
                                        ::QueryResponse* response) {
    std::unique_lock<std::mutex> guard(this->vect_mtx);
    for (my_server server : this->server_vect){
        auto server_entry = response->add_config();
        server_entry->set_server(server.address);
        //make shard list
        if (server.server_shards.size() != 0){
            for (shard shard : server.server_shards){
                auto addShard = server_entry->add_shards();
                addShard->set_lower(shard.lower);
                addShard->set_upper(shard.upper);
            }
        }
    }
    return ::grpc::Status::OK;
}
