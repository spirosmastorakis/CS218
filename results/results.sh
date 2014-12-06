#!/usr/bin/bash         

requests=0
delivered=0
delay=0
average_delay=0
interval=0
temp_sum=0
temp_num=0
cost=0
average_cost=0

filename="$1"

while read line;
do 
	if [[ "$interval" == "1" ]]
	then
		temp_num=$(echo "scale=3; $line - 0.5" | bc)
		temp_sum=$(echo "scale=3; $temp_num + $temp_sum" | bc)
		interval=0
		read line1
		next_line=$((line1))
		read line2
		after_next_line=$((line2))
		cost=$((next_line+after_next_line+cost))
	fi	
	if [[ "$line" == *"requests content:"* ]]
	then
		requests=$((requests+1))
	fi
    	if [[ "$line" == "SUCCESS FIRST"* ]]
    	then
		delivered=$((delivered+1))
		
	fi
    	if [[ "$line" == "Time now"* ]] 
	then
		interval=1
	fi		
done < $1

average_delay=$(echo "scale=3; ($temp_sum / $delivered)" | bc)
delivery_ratio=$(echo "scale=3; $delivered / $requests * 100" | bc)
average_cost=$(echo "scale=3; $cost / $delivered" | bc)
echo "Requests = " ${requests} > txt.txt
echo "Delivered = " ${delivered} >> txt.txt
echo "Delivery ratio = " ${delivery_ratio} " %" >> txt.txt
echo "Average delay = " ${average_delay} " sec" >> txt.txt
echo "Average Cost = " ${average_cost} " packets" >> txt.txt
