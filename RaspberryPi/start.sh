# check to see if libboost-python is installed
count=$(dpkg -s libboost-python1.62.0 | egrep -c "Package: libboost-python1.62.0")
if [ ${count} != "1" ]
then
  sudo apt-get install libboost-python1.62.0
fi

cd src
python ./main.py
