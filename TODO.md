


## perf
sudo sysctl -w kernel.perf_event_paranoid=1
perf record -g --call-graph dwarf ./bin/relWithDebInfo/test/stream
heaptrack ./bin/relWithDebInfo/test/stream

3.1. Flame Graph

This is the most essential feature of Hotspot.

    X‑axis: Represents the width of the call stack. A wider bar means that function consumes more CPU time.

    Y‑axis: Represents the depth of the call stack, showing the call hierarchy from bottom (root) to top (leaf).

    Interaction:

        Hover: Hover your mouse over any coloured block, and a tooltip will appear showing the function name, time consumed, and percentage.

        Click: Click on any coloured block to zoom in and examine the internal calls of that function in detail.

        Search: There is usually a search box on the interface where you can type a function name to locate it quickly.

3.2. Other Data Tables

In addition to the flame graph, Hotspot offers several table views to help you analyse the data from different angles:

    Top‑Down: Starts from the top‑level callers and drills down layer by layer to see where CPU time is spent.

    Bottom‑Up: Starts from the lowest‑level functions (leaf functions) and traces backwards to see who called them.

    Caller‑Callee: For a selected function, it shows both its callers and its callees simultaneously.

## Next steps

1. Remove multithreading.
   Create a vector containing the entire file.
   Pass this vector to a HTMLParser object.
   Each stage has access to a context object containing all other stages and calls process on the next stage directly.
   This makes stream a lot simpler and removes a bunch of buffers reducing memory consumption and simplifying the design.

2. Find a better alternative to search for chars in sets in the preprocessor.
3. Check the difference in performance now.

4. The decoder may be better implemented using a lookup table.
5. Check perf difference.

6. Maybe find a way to remove transactional streams.