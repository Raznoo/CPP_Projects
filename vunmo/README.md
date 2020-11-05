Project 4 - Vunmo
====================

<!-- TODO: Fill this out. -->
## Design Overview:
i couldn't figure out where my tests for invalid clients went wrong :(
## Collaborators:
NONE
## Conceptual Questions:
1) For smaller client numbers, a fixed number of worker threads would work (roughly) similar to
a worker per client. It becomes a problem whenever you have something of a much larger scale
(think google). If every client that was querying a large server had to be shared between say 5
workers, each client would have a horrendous amount of downtime while it waited for its turn to
run. The benefit of having 5 worker threads is it is much easier to debug 5 threads versus a thousand.
2) a client will send a request to the server to withdraw from their
account. This request gets added to the work queue in the server for
one of the workers to pick up (eventually) a worker thread will then
use the information that the client provided to the work queue to process
the withdraw request. Whether the withdrawal was successful or otherwise, a response
will then be sent from the server to the client.
3) If the thread representing zachary locks his thread, then is interrupted by alice.
when alice's thread executes, it will first lock its own thread, then attempt to lock zachary's.
However, because zachary's thread is already locked, this will result in zachary waiting for
alice to unlock and alic waiting for zachary to unlock. Neither one of them will give up their lock
so this is a perfect example of deadlock.
This can be solved by deciding which process to go first by comparing something both threads 
have in common. If you compare both of the ID's of the user, it can't be the case that
they are the same. Therefore, if for both threads, you lock the thread with the smallest of the two IDs 
first, then this helps to avoid deadlock.
## How long did it take to complete Vunmo?

