svn info
uname -a

for i in 1 2 3 4 5 6 7 8 9 10
do
	/usr/bin/time -v ./blackscholes 5000000 100
	echo					
done
