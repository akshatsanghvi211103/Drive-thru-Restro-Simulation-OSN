# Pizzeria

## Follow up questions:
> We Create a semaphore for the pickup spot, and if the pickup spot is occupied, then the chef will route the order to the secondary storage. The time spent by an order waiting on the semaphore would be represented as the time spent by the order in secondary storage. The order of wakeup for the waiting processes would be FCFS.
> The drive thru would only reject due to the unavailability of the chef, since the ingredients can be theoretically assumed to be infinite. The manager would decide the rejectance due to unavailability of chef.