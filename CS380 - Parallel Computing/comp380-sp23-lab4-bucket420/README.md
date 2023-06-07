## Performance measurement
### Data
|p|time (s)|speed-up|efficiency|
|:----|:----|:----|:----|
|1|22120.8812|1|1|
|8|1920.7068|11.51705258|1.439631572|
|48|502.8597|43.99016505|0.916461772|
|96|251.5214|87.94830659|0.916128194|
|192|115.6574|191.2621345|0.996156951|
|480|38.3992|576.0766162|1.200159617|
|1056|33.8453|653.5879782|0.61892801|
|1680|8.3257|2656.939501|1.581511607|
|2212|19.2073|1151.691347|0.520656124|

### Execution time vs Number of processes
![image](https://user-images.githubusercontent.com/72154050/231851294-feb9be7b-619d-402e-a4dc-f3c6605345ca.png)

### Speed-up vs Number of processes
![image](https://user-images.githubusercontent.com/72154050/231851545-e551e13f-9fef-4708-8cd6-e5e946693231.png)

### Efficiency vs Number of processes
![image](https://user-images.githubusercontent.com/72154050/231851754-d25d6d07-76b2-4ec1-a26f-8fc344ff2818.png)

## Performance analysis
In general, the program runs faster as the number of processes increases. However, the speed-up is not really linear because the execution time also depends on the location of the password in the search space. Some values of p performs exceptionally well with superlinear efficiency because the password just happens to be near the begining of the local search. When this is not the case, the efficiency is much lower for large p because collective communications are expensive.
