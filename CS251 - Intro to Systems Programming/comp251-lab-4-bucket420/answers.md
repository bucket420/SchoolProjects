## Phase 1:
   ### Modifications:  
   ``` 
   print *params.n = 256
   ```
   ### How to find the memory corruption:
   
   After the program crashes, run:
   ```
   print arr[i]
   ```
   This returns an error message saying &arr[i] is not a valid address, which suggests that i is out of range, and `*params.n` is greater than it's supposed to be. We can verify this by running:
   ```
   print params.mm_region
   print *params.n
   ```
   The commands return 2064 and 4096, respectively. The size of the array is indeed much
   greater than what the file can store. The correct size should be
   (params.mm_region - 2 * sizeof(uint64_t)) / sizeof(unint64_t) = 256

## Phase 2:
   ### Modifications:
   ```
   print nav_db->metadata->size = 65536
   print *((char*) 0x7ffff7ff6eea + 21 - 1) = '\0'
   ```
   ### How to find the memory corruption:
   
   When the program crashes, the error message `Error: tbl->metadata->size ==
   tbl->mm_region.size` suggests that the size in the metadata and the size of
   the file don't match. To get more info, run:
   ```
   info stack
   frame 1
   print nav_db->metadata->size
   print nav_db->mm_region.size
   ```
   This returns 1048576 and 65536, respectively. Thus, we need to set
   `nav_db->metadata->size` to 65536.
   
   That was not the only error, however. When we run the program again, there's another crash with the message 
   `Assertion 'el_len == strlen(tmp) + 1' failed`, which means the length we get
   by subtracting offsets doesn't match `strlen(temp) + 1`. To investigate this, run:
   ```
   info stack
   frame 4
   print tmp
   print el_len
   ```
   This returns `0x7ffff7ff6eea "326.4;0.9;2735;P1182\377\061\067\064.9014;16.8813;558;NGC"` and
   21. Since the number of characters is clearly greater than 21, it seems the string is not properly null-terminated. To fix this, we must set the 21th character to `'\0'`.

## Phase 3:
   ### Modifications:
   ```
   print nav_db->metadata->len = 33
   ```
   ### How to find the memory corruption:
   
   The program crashes due to a segfault, suggesting that there's an invalid
   pointer somewhere. To get more info, run:
   ```
   info stack
   frame 1
   print i
   ```
   This returns 33, which means we are accessing the 34th element (because we started from 0). Since `print get_element(&nav_db, 33)` results in an
   invalid pointer error, the correct number of elements must be 34 - 1 = 33 (the previous iteration ran fine). However,
   `print nav_db->metadata->len` returns 2121, so we must set it to 33.
   
## Phase 4:
   ### Modifications:
   ```
   print *((uint64_t *)(flight_log.mm_region.start + flight_log.mm_region.size - 8)) = 0
   ```
   ### How to find the corruption:
   
   The program returns a segfault the start of the while loop in `init_tail`. If we try `print *((uint32_t*) start)`, an invalid pointer error appears,
   which suggests that the loop didn't end when it was supposed to. In other
   words, the loop continued after passing through the tail. This means the tail was not zero, and we must set it to zero.
   

## Phase 5:
   ### Modifications:
   ```
   print *(uint32_t*)(cur + cur_size) = 437
   ```
   ### How to find the memory corruption:
   
   When the program crashes, `info stack` shows that the segfault occurs at line
   191 of boot.c. If we go to this frame and try `print cur`, an invalid pointer
   error appears. This means `bl_prev` in the previous iteration of the loop returned the wrong address, which suggests that `cur_size` - the value stored at the     footer - might be wrong. By running `print cur_size`, we know that this value is 511. However, the offset between the footer and the start of the data structure, as returned by `print cur + cur_size - flight_log.mm_region.start`, is only 449, so `cur_size` is indeed wrong. Looking at the output of the program before the crash, we can also see that `cur` is the first element. The correct size is therefore 449 - 8 - 4 = 437 (excluding the empty block and the header). We can verify that this is also the value stored at the header by running `print *(uint32_t *)(flight_log.mm_region.start + 8)`. 



