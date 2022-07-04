max=9
for j in 100 200 300 400
do
	for i in `seq 0 $max`
	do
		optee_example_cv ${j}
	done
done
