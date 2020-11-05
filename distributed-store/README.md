Project 5: Distributed Systems
==============================

# How to Run: 
run `./setup.sh` to install grpc and its dependencies - answer yes to any
prompts it gives.

to build your code run `make` from inside the build directory.

If you want to add new files, edit the CMakeLists.txt file to include your
new source files in any targets they are use by. Then `cd` into the build
directory and run `cmake ..` before running `make` again, to update the
makefile with your new files.

The clients are provided in the `clients` directory - running them will give
you instructions on how to use them.

<!-- TODO: Fill this out. -->
## Design Overview:

## Collaborators:

## Conceptual Questions:
1) The controller regulates the traffic to and from the server. It is responsible for the
    balancing of shards across all servers as well as storing how a client finds the server
    containing the key that it is looking for
    
    The client is the accessor of the data. They will send a request to the controller (or server
    depending on what the operation is and whether or not a server was previously found) and recieve
    the requested data or an error from either the controller or server
    
    The server is a container for shards. This is where all of the data associated with a particular
    key will be stored. It is only a storage area for data that can be accessed later.
    
    in a client request for k, the request will be passed to the controller who will access the
    appropriate server that contains a shard that contains k. The server will respond to the controller
    with either an error or the value associated with k and that will then be passed onto the client
2) It makes it much easier to use our database from many areas. If there's a user who will be using our
    client interface in a codebase that is written in Java and be talking to our database which is written
    in C++, it is much easier to use a protobuf rather that trying to create code custom tailored to their
    very specific codebase so they can use our database. After creating this custom code, any additions or
    updates to our database would also require additions and updates to every single user of our database.
    This is different than a protobuf where if we want to update something all we do is send them a new file
    to ensure their code works with ours.
    TL DR: its the difference between adding a file to make sure their code is compliant, or a completely
    new, potentially expensive and time consuming project to make sure the user can use our database.
3) Because with every request to the shardmaster, the shardmaster will lock up, preventing any other
    clients accessing the shardmaster until the first client gets his response back. This effectively just
    turns the shardmaster into one really slow server with a scalable amount of storage.
4) This requires much more storage on the server side but if you map each key to a (value, int) tuple, where
    value is the data and the int is a frequency count of the accesses to the key. There should be some
    upper limit of the frequency when the key is accessed compared to a total access counter of all keys 
    when a key is considered "hot" if it goes over that limit. Then, similarly extra keys are dispersed among the
    servers evenly when the total keys don't evenly divide, the hot keys should be explicitly dispersed
    among the servers every time the keys are rebalanced by making an explicit shard for that one key. This
    would be faster because each server has at worst one more "hot" key than any other server. This evenly
    distributes the load across all servers within a tolerance that the shardmaster sets.
## How long did it take to complete Distributed Systems?
18 - 24 hours