There are two versions of N-body simulation in this repo:  
* `lab3` (default): body data are stored in a 2d array 
* `lab3_aos`: body data are stored in an array of structs 
 

`lab3` has worse abstraction but better performance because it requires less remote access, as shown in the `output` folder:  
   Version    | N = 300K, P = 480 | N = 300K, P = 960 | N = 300K, P = 1680 | N = 300K, P = 2112 | N = 2M, P = 1680 | N = 2M, P = 2112 |
------------- | ------------------| ----------------- | ------------------ | ------------------ | -----------------| ---------------- |
`lab3`        | 1341.282 ms       | 982.024 ms        | 774.940 ms         | 715.010 ms         | 12900.907 ms     | 10899.819 ms     |
`lab3_aos`    | 1474.172 ms       | 1206.462 ms       | 920.612 ms         | 821.362 ms         | 14999.041 ms     | 13296.113 ms     |
